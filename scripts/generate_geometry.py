"""Generate the minimal Montreal detector GDML from documented dimensions."""

import xml.etree.ElementTree as ET
import tomllib
from math import cos, pi, sin
from pathlib import Path

CONFIG_PATH = Path(__file__).resolve().parent.parent / "config" / "configuration.toml"
with CONFIG_PATH.open("rb") as config_file:
    CONFIG = tomllib.load(config_file)
GEOMETRY = CONFIG["geometry"]

N_BARS = int(GEOMETRY["scintillator_count"])
BAR_LENGTH_CM = float(GEOMETRY["scintillator_length_cm"])
BAR_INNER_RADIUS_CM = float(GEOMETRY["scintillator_inner_radius_cm"])
BAR_RADIAL_THICKNESS_CM = float(GEOMETRY["scintillator_radial_thickness_cm"])
BAR_TANGENTIAL_WIDTH_CM = float(GEOMETRY["scintillator_tangential_width_cm"])
MWPC_INNER_RADIUS_CM = float(GEOMETRY["mwpc_inner_radius_cm"])
MWPC_LENGTH_CM = float(GEOMETRY["mwpc_length_cm"])
MWPC_WALL_THICKNESS_CM = float(GEOMETRY["mwpc_wall_thickness_cm"])
ROHACELL_DENSITY_G_CM3 = float(GEOMETRY["rohacell_density_g_cm3"])
PMT_RADIUS_CM = float(GEOMETRY["pmt_radius_cm"])
PMT_LENGTH_CM = float(GEOMETRY["pmt_length_cm"])
BEAMLINE_LENGTH_CM = float(GEOMETRY["beamline_length_cm"])
BEAMLINE_OUTER_RADIUS_CM = float(GEOMETRY["beamline_outer_radius_cm"])
BEAMLINE_WALL_CM = float(GEOMETRY["beamline_wall_cm"])
TARGET_RADIUS_CM = float(GEOMETRY["target_radius_cm"])
AL_FOIL_THICKNESS_CM = float(GEOMETRY["al_foil_thickness_cm"])
LIF_THICKNESS_CM = float(GEOMETRY["lif_thickness_cm"])
TARGET_LAYER_GAP_CM = float(GEOMETRY["target_layer_gap_cm"])
COOLING_ROD_RADIUS_CM = float(GEOMETRY["cooling_rod_radius_cm"])
COOLING_ROD_LENGTH_CM = float(GEOMETRY["cooling_rod_length_cm"])
COOLING_ROD_CENTER_Y_CM = float(GEOMETRY["cooling_rod_center_y_cm"])


def node(parent, tag, **attributes):
    return ET.SubElement(
        parent, tag, {key: str(value) for key, value in attributes.items()}
    )


def main():
    root = ET.Element("gdml")
    node(root, "define")
    materials = node(root, "materials")

    for name, symbol, z, mass in (
        ("Nitrogen", "N", 7, 14.0067),
        ("Oxygen", "O", 8, 15.999),
        ("Carbon", "C", 6, 12.011),
        ("Argon", "Ar", 18, 39.948),
        ("Hydrogen", "H", 1, 1.008),
        ("Silicon", "Si", 14, 28.085),
        ("Fluorine", "F", 9, 18.998403),
        ("Aluminum", "Al", 13, 26.981538),
        ("Copper", "Cu", 29, 63.546),
    ):
        element = node(materials, "element", name=name, formula=symbol, Z=z)
        node(element, "atom", value=mass)

    lithium7 = node(materials, "isotope", name="Lithium7Isotope", Z=3, N=7)
    node(lithium7, "atom", value=7.016004)
    lithium7_element = node(materials, "element", name="Lithium7", formula="Li")
    node(lithium7_element, "fraction", n=1.0, ref="Lithium7Isotope")

    def mixture(name, density, fractions, state=None):
        attributes = {"name": name}
        if state:
            attributes["state"] = state
        material = node(materials, "material", **attributes)
        node(material, "D", value=density, unit="g/cm3")
        for element, fraction in fractions:
            node(material, "fraction", n=fraction, ref=element)

    mixture("Air", 0.001225, [("Nitrogen", 0.7), ("Oxygen", 0.3)], "gas")
    # 74% Ar / 26% CO2 by volume, converted to elemental mass fractions.
    mixture(
        "MWPCGas",
        0.001834,
        [
            ("Argon", 0.720944809),
            ("Carbon", 0.076160147),
            ("Oxygen", 0.202895044),
        ],
        "gas",
    )
    # Material compositions are starter approximations, not a calibrated model.
    mixture(
        "PlasticScintillator",
        1.032,
        [
            ("Carbon", 0.915),
            ("Hydrogen", 0.085),
        ],
    )
    # Effective PMI foam approximation. Density depends on the Rohacell grade.
    mixture(
        "Rohacell",
        ROHACELL_DENSITY_G_CM3,
        [
            ("Carbon", 0.6273),
            ("Hydrogen", 0.0724),
            ("Nitrogen", 0.0914),
            ("Oxygen", 0.2089),
        ],
    )
    mixture("Glass", 2.5, [("Silicon", 0.467), ("Oxygen", 0.533)])
    mixture("Vacuum", 1e-25, [("Hydrogen", 1.0)], "gas")
    mixture("CarbonFiber", 1.6, [("Carbon", 1.0)])  # effective approximation
    mixture("AlFoil", 2.70, [("Aluminum", 1.0)])
    mixture("CoolingCopper", 8.96, [("Copper", 1.0)])
    mixture("Lithium7Fluoride", 2.64, [
        ("Lithium7", 7.016004 / (7.016004 + 18.998403)),
        ("Fluorine", 18.998403 / (7.016004 + 18.998403)),
    ])

    solids = node(root, "solids")
    node(solids, "box", name="WorldBox", x=200, y=200, z=200, lunit="cm")
    node(
        solids,
        "box",
        name="ScintillatorBar",
        x=BAR_RADIAL_THICKNESS_CM,
        y=BAR_TANGENTIAL_WIDTH_CM,
        z=BAR_LENGTH_CM,
        lunit="cm",
    )
    node(
        solids,
        "tube",
        name="MWPCGasCylinder",
        rmin=0,
        rmax=MWPC_INNER_RADIUS_CM,
        z=MWPC_LENGTH_CM,
        startphi=0,
        deltaphi=360,
        aunit="deg",
        lunit="cm",
    )
    # Transverse clearance through the chamber for the x-axis beamline.
    node(solids, "tube", name="BeamlineClearance", rmin=0,
         rmax=BEAMLINE_OUTER_RADIUS_CM + 0.001,
         z=BEAMLINE_LENGTH_CM + 0.2, startphi=0, deltaphi=360,
         aunit="deg", lunit="cm")
    node(solids, "tube", name="BeamlineVacuumCylinder", rmin=0,
         rmax=BEAMLINE_OUTER_RADIUS_CM - BEAMLINE_WALL_CM,
         z=BEAMLINE_LENGTH_CM, startphi=0, deltaphi=360,
         aunit="deg", lunit="cm")
    node(solids, "tube", name="BeamlineCarbonShell", 
         rmin=BEAMLINE_OUTER_RADIUS_CM - BEAMLINE_WALL_CM,
         rmax=BEAMLINE_OUTER_RADIUS_CM, z=BEAMLINE_LENGTH_CM,
         startphi=0, deltaphi=360, aunit="deg", lunit="cm")
    for name, radius, thickness in (
        ("AlBackingDisk", TARGET_RADIUS_CM, AL_FOIL_THICKNESS_CM),
        ("Lithium7FluorideDisk", TARGET_RADIUS_CM, LIF_THICKNESS_CM),
        ("CoolingRodCylinder", COOLING_ROD_RADIUS_CM, COOLING_ROD_LENGTH_CM),
    ):
        node(solids, "tube", name=name, rmin=0, rmax=radius,
             z=thickness, startphi=0, deltaphi=360,
             aunit="deg", lunit="cm")
    node(
        solids,
        "tube",
        name="MWPCWallCylinder",
        rmin=MWPC_INNER_RADIUS_CM,
        rmax=MWPC_INNER_RADIUS_CM + MWPC_WALL_THICKNESS_CM,
        z=MWPC_LENGTH_CM,
        startphi=0,
        deltaphi=360,
        aunit="deg",
        lunit="cm",
    )
    for name, first in (("MWPCGasWithBeamPort", "MWPCGasCylinder"),
                        ("MWPCWallWithBeamPort", "MWPCWallCylinder")):
        cut = node(solids, "subtraction", name=name)
        node(cut, "first", ref=first)
        node(cut, "second", ref="BeamlineClearance")
        node(cut, "rotation", name=f"{name}_rotation", x=0, y=90, z=0, unit="deg")
    node(
        solids,
        "tube",
        name="PMTCylinder",
        rmin=0,
        rmax=PMT_RADIUS_CM,
        z=PMT_LENGTH_CM,
        startphi=0,
        deltaphi=360,
        aunit="deg",
        lunit="cm",
    )

    structure = node(root, "structure")

    def volume(name, material, solid):
        result = node(structure, "volume", name=name)
        node(result, "materialref", ref=material)
        node(result, "solidref", ref=solid)
        return result

    volume("MWPCGas", "MWPCGas", "MWPCGasWithBeamPort")
    volume("MWPCWall", "Rohacell", "MWPCWallWithBeamPort")
    volume("BeamlineWall", "CarbonFiber", "BeamlineCarbonShell")
    volume("AlBacking", "AlFoil", "AlBackingDisk")
    volume("Lithium7FluorideTarget", "Lithium7Fluoride", "Lithium7FluorideDisk")
    volume("CoolingRod", "CoolingCopper", "CoolingRodCylinder")
    beam_vacuum = volume("BeamlineVacuum", "Vacuum", "BeamlineVacuumCylinder")
    volume("Scintillator", "PlasticScintillator", "ScintillatorBar")
    volume("PMT", "Glass", "PMTCylinder")
    world = volume("World", "Air", "WorldBox")

    def place(name, logical, copy_number, x=0.0, y=0.0, z=0.0,
              angle=0.0, parent=None, rotation_x=0.0, rotation_y=0.0):
        physical = node(parent if parent is not None else world,
                        "physvol", name=name, copynumber=copy_number)
        node(physical, "volumeref", ref=logical)
        node(
            physical,
            "position",
            name=f"{name}_position",
            x=f"{x:.9f}",
            y=f"{y:.9f}",
            z=f"{z:.9f}",
            unit="cm",
        )
        if angle or rotation_x or rotation_y:
            node(
                physical,
                "rotation",
                name=f"{name}_rotation",
                x=f"{rotation_x:.6f}",
                y=f"{rotation_y:.6f}",
                z=f"{angle:.6f}",
                unit="deg",
            )

    place("MWPCGas", "MWPCGas", 0)
    place("MWPCWall", "MWPCWall", 0)
    # GDML placement rotation -90 deg about y maps local +z to world +x.
    place("BeamlineVacuum", "BeamlineVacuum", 0, rotation_y=-90)
    place("BeamlineWall", "BeamlineWall", 0, rotation_y=-90)
    # Local coordinates below are relative to the beamline vacuum.
    # The LiF coating is centered at the origin, facing protons along +x.
    coating_offset = ((AL_FOIL_THICKNESS_CM + LIF_THICKNESS_CM) / 2
                      + TARGET_LAYER_GAP_CM) / 2**0.5
    place("Lithium7FluorideTarget", "Lithium7FluorideTarget", 0,
          parent=beam_vacuum, rotation_y=-45)
    place("AlBacking", "AlBacking", 0, x=coating_offset, z=coating_offset,
          parent=beam_vacuum, rotation_y=-45)
    place("CoolingRod", "CoolingRod", 0, y=COOLING_ROD_CENTER_Y_CM,
          parent=beam_vacuum, rotation_x=90)
    radius = BAR_INNER_RADIUS_CM + BAR_RADIAL_THICKNESS_CM / 2
    pmt_z = BAR_LENGTH_CM / 2 + PMT_LENGTH_CM / 2
    for bar_index in range(N_BARS):
        label = bar_index + 1
        angle = 2 * pi * bar_index / N_BARS
        x, y = radius * cos(angle), radius * sin(angle)
        # GDML's rotation is the inverse of the placement rotation here.
        place(
            f"Scintillator_{label:02d}",
            "Scintillator",
            label,
            x,
            y,
            angle=-bar_index * 360 / N_BARS,
        )
        # Confirmed convention: D = -z, U = +z.
        for end, z, channel in (("D", -pmt_z, 2 * label - 1), ("U", pmt_z, 2 * label)):
            place(f"PMT_{label:02d}_{end}", "PMT", channel, x, y, z)

    setup = node(root, "setup", name="Default", version="1.0")
    node(setup, "world", ref="World")
    ET.indent(root, space="  ")
    path = CONFIG_PATH.parent / CONFIG["runtime"]["geometry_file"]
    path.parent.mkdir(parents=True, exist_ok=True)
    ET.ElementTree(root).write(path, encoding="unicode", xml_declaration=True)


if __name__ == "__main__":
    main()
