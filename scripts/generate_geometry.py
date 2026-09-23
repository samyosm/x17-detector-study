"""Generate the active WC_Scint_16dE_x/B4c geometry (not its backup variants).

Lengths are full extents in cm. Rotations are Geant4 frame rotations, as in
GDML. Deliberately retain the reference's gaps, absent foil and approximate pi.
"""
import math
from pathlib import Path
import tomllib
import xml.etree.ElementTree as ET

CONFIG_PATH = Path(__file__).resolve().parents[1] / "config/configuration.toml"


def node(parent, tag, **attributes):
    return ET.SubElement(parent, tag, {k: str(v) for k, v in attributes.items()})


def generate(config):
    g = config["geometry"]
    root = ET.Element("gdml")
    node(root, "define")
    materials = node(root, "materials")
    for name, symbol, z, mass in (
        ("Carbon", "C", 6, 12.01), ("Hydrogen", "H", 1, 1.01),
        ("Nitrogen", "N", 7, 14.006855), ("Oxygen", "O", 8, 15.99940),
        ("Argon", "Ar", 18, 39.95),
    ):
        element = node(materials, "element", name=name, formula=symbol, Z=z)
        node(element, "atom", value=mass)

    def material(name, density, components, atoms=False):
        result = node(materials, "material", name=name)
        node(result, "D", value=density, unit="g/cm3")
        for ref, amount in components:
            node(result, "composite" if atoms else "fraction", n=amount, ref=ref)
        return result

    vacuum = material("Galactic", 1e-25, [("Hydrogen", 1.0)])
    vacuum.set("state", "gas")
    node(vacuum, "T", value=2.73, unit="K")
    node(vacuum, "P", value=3e-18, unit="pascal")
    material("PLA", 1.24, [("Carbon", 3), ("Hydrogen", 4), ("Oxygen", 2)], True)
    material("fDCgas", 0.0017, [("Argon", .8), ("G4_CARBON_DIOXIDE", .2)])
    # Their fractions sum to 1.0001; Geant4 normalizes them. Keep the inputs.
    material("Rohacell", .100, [("Nitrogen", .0839), ("Oxygen", .1914),
                                ("Carbon", .6464), ("Hydrogen", .0784)])
    solids = node(root, "solids")
    structure = node(root, "structure")

    def volume(name, material_name, solid):
        result = node(structure, "volume", name=name)
        node(result, "materialref", ref=material_name)
        node(result, "solidref", ref=solid)
        return result

    def box(name, material_name, x, y, z):
        node(solids, "box", name=name + "Solid", x=x, y=y, z=z, lunit="cm")
        return volume(name, material_name, name + "Solid")

    def tube(name, material_name, rmin, rmax, length):
        node(solids, "tube", name=name + "Solid", rmin=rmin, rmax=rmax,
             z=length, startphi=0, deltaphi=2 * math.pi, aunit="rad", lunit="cm")
        return volume(name, material_name, name + "Solid")

    def place(parent, logical, name=None, copy=0, xyz=(0, 0, 0), rotation=(0, 0, 0)):
        name = name or logical.get("name")
        result = node(parent, "physvol", name=name, copynumber=copy)
        node(result, "volumeref", ref=logical.get("name"))
        node(result, "position", name=name + "Position", unit="cm",
             **dict(zip(("x", "y", "z"), xyz)))
        node(result, "rotation", name=name + "Rotation", unit="rad",
             **dict(zip(("x", "y", "z"), rotation)))

    chamber = tube("MWPCGas", "fDCgas", g["mwpc_inner_radius_cm"],
                   g["mwpc_outer_radius_cm"], g["mwpc_length_cm"])
    inner = tube("MWPCInnerWall", "Rohacell", g["mwpc_inner_radius_cm"],
                 g["mwpc_inner_radius_cm"] + g["mwpc_wall_thickness_cm"], g["mwpc_length_cm"])
    outer = tube("MWPCOuterWall", "Rohacell", g["mwpc_outer_radius_cm"] - g["mwpc_wall_thickness_cm"],
                 g["mwpc_outer_radius_cm"], g["mwpc_length_cm"])
    place(chamber, inner)
    place(chamber, outer)
    pipe = tube("BeamlineWall", "G4_GRAPHITE", g["beamline_inner_radius_cm"],
                g["beamline_outer_radius_cm"], g["beamline_length_cm"])
    vacuum = tube("BeamlineVacuum", "Galactic", 0, g["beamline_vacuum_radius_cm"], g["beamline_length_cm"])
    target = tube("LithiumFluorideTarget", "G4_LITHIUM_FLUORIDE", 0,
                  g["target_radius_cm"], g["lif_thickness_cm"])
    place(vacuum, target, xyz=(0, 0, g["target_z_cm"]), rotation=(math.pi / 4, 0, 0))
    nut_size = g["nut_size_cm"]
    rod_y = g["cooling_rod_center_y_cm"]
    rod_short = rod_y - g["cooling_rod_outer_radius_cm"] - nut_size[1] - nut_size[0] / 2
    rod_half = g["beamline_length_cm"] / 4 - rod_short
    rod = tube("CoolingRod", "G4_Cu", g["cooling_rod_inner_radius_cm"], g["cooling_rod_outer_radius_cm"], 2 * rod_half)
    place(vacuum, rod, xyz=(0, rod_y, -(rod_half + rod_short)))
    nut = box("BrassNut", "G4_BRASS", *nut_size)
    place(vacuum, nut, xyz=(0, rod_y - g["cooling_rod_outer_radius_cm"] - nut_size[1] / 2,
                           -(rod_short + nut_size[2] / 2)))
    flange = tube("Flange", "G4_Al", g["flange_inner_radius_cm"],
                  g["flange_outer_radius_cm"], g["flange_length_cm"])

    n = g["scintillator_count"]
    if n != 16:
        raise ValueError("The reference readout requires 16 scintillators")
    theta = math.pi / n
    radius = g["scintillator_inner_radius_cm"]
    thickness = g["scintillator_radial_thickness_cm"]
    length = g["scintillator_length_cm"]
    wrap = g["scintillator_wrap_thickness_cm"]
    poly = node(solids, "polyhedra", name="ScintillatorWrapSolid", startphi=theta,
                deltaphi=2 * math.pi + theta, numsides=n, aunit="rad", lunit="cm")
    for z in (-wrap, length + wrap):
        node(poly, "zplane", rmin=radius - wrap, rmax=radius + thickness + wrap, z=z)
    barrel = volume("ScintillatorWrap", "G4_POLYVINYL_CHLORIDE", "ScintillatorWrapSolid")
    node(solids, "trd", name="ScintillatorSolid", lunit="cm",
         x1=2 * ((radius + thickness) * math.tan(theta) - wrap),
         x2=2 * (radius * math.tan(theta) - wrap), y1=length, y2=length, z=thickness)
    scint = volume("Scintillator", "G4_POLYSTYRENE", "ScintillatorSolid")
    # World must follow its daughters for the GDML reader.
    world = box("World", "G4_AIR", *([g["world_size_cm"]] * 3))
    for logical in (chamber, pipe, vacuum):
        place(world, logical)
    for sign, name in ((-1, "FlangeNegative"), (1, "FlangePositive")):
        place(world, flange, name, xyz=(0, 0, sign * (g["beamline_length_cm"] + g["flange_length_cm"]) / 2))
    place(world, barrel, xyz=(0, 0, -length / 2))
    for i in range(n):
        phi = i * 2 * theta
        r = radius + thickness / 2
        # Reference builds Rx(3.1416/2) Rz(phi); GDML builds Rz Ry Rx.
        a = 3.1416 / 2
        frame_rotation = (
            math.atan2(math.sin(a) * math.cos(phi), math.cos(a)),
            -math.asin(math.sin(a) * math.sin(phi)),
            math.atan2(math.cos(a) * math.sin(phi), math.cos(phi)),
        )
        place(barrel, scint, f"Scintillator_{i:02d}", i,
              (-r * math.sin(phi), -r * math.cos(phi), length / 2),
              frame_rotation)
        housing = box(f"DeltaEHousing_{i:02d}", "PLA", *g["delta_e_housing_size_cm"])
        cavity = box(f"DeltaECavity_{i:02d}", "G4_AIR", *g["delta_e_cavity_size_cm"])
        de = box(f"DeltaEScintillator_{i:02d}", "G4_POLYSTYRENE", *g["delta_e_size_cm"])
        place(cavity, de, copy=i)
        place(housing, cavity, copy=i)
        rde = g["delta_e_radius_cm"]
        place(world, housing, copy=i, xyz=(-rde * math.sin(phi), -rde * math.cos(phi), 0),
              rotation=(0, 0, math.pi / 2 + phi))
    # Topologically order volumes, including nested assemblies.
    ordered = []
    seen = set()
    volumes = {v.get("name"): v for v in structure}
    def visit(v):
        if v.get("name") in seen:
            return
        for ref in v.findall("physvol/volumeref"):
            visit(volumes[ref.get("ref")])
        seen.add(v.get("name"))
        ordered.append(v)
    visit(world)
    structure[:] = ordered
    setup = node(root, "setup", name="Default", version="1.0")
    node(setup, "world", ref="World")
    ET.indent(root, space="  ")
    return ET.ElementTree(root)


def main():
    with CONFIG_PATH.open("rb") as stream:
        config = tomllib.load(stream)
    path = CONFIG_PATH.parent / config["runtime"]["geometry_file"]
    path.parent.mkdir(parents=True, exist_ok=True)
    generate(config).write(path, encoding="unicode", xml_declaration=True)


if __name__ == "__main__":
    main()
