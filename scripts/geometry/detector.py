import math
from .gdml import GdmlBuilder, add_element


def build_materials(builder):
    for name, symbol, z, mass in (
        ("Carbon", "C", 6, 12.01), ("Hydrogen", "H", 1, 1.01),
        ("Nitrogen", "N", 7, 14.006855), ("Oxygen", "O", 8, 15.99940),
        ("Argon", "Ar", 18, 39.95),
    ):
        element = add_element(builder.materials, "element", name=name, formula=symbol, Z=z)
        add_element(element, "atom", value=mass)

    vacuum = builder.add_material("Galactic", 1e-25, [("Hydrogen", 1.0)])
    vacuum.set("state", "gas")
    add_element(vacuum, "T", value=2.73, unit="K")
    add_element(vacuum, "P", value=3e-18, unit="pascal")
    builder.add_material("PLA", 1.24, [("Carbon", 3), ("Hydrogen", 4), ("Oxygen", 2)], True)
    builder.add_material("fDCgas", 0.0017, [("Argon", .8), ("G4_CARBON_DIOXIDE", .2)])

    builder.add_material("Rohacell", .100, [("Nitrogen", .0839), ("Oxygen", .1914),
                                ("Carbon", .6464), ("Hydrogen", .0784)])


def build_tracking_chamber(builder, dimensions):
    chamber = builder.add_tube("MWPCGas", "fDCgas", dimensions["mwpc"]["inner_radius_cm"],
                   dimensions["mwpc"]["outer_radius_cm"], dimensions["mwpc"]["length_cm"])
    inner = builder.add_tube("MWPCInnerWall", "Rohacell", dimensions["mwpc"]["inner_radius_cm"],
                 dimensions["mwpc"]["inner_radius_cm"] + dimensions["mwpc"]["wall_thickness_cm"], dimensions["mwpc"]["length_cm"])
    outer = builder.add_tube("MWPCOuterWall", "Rohacell", dimensions["mwpc"]["outer_radius_cm"] - dimensions["mwpc"]["wall_thickness_cm"],
                 dimensions["mwpc"]["outer_radius_cm"], dimensions["mwpc"]["length_cm"])
    builder.place(chamber, inner)
    builder.place(chamber, outer)
    return chamber


def build_beamline(builder, dimensions):
    pipe = builder.add_tube("BeamlineWall", "G4_GRAPHITE", dimensions["beamline"]["inner_radius_cm"],
                dimensions["beamline"]["outer_radius_cm"], dimensions["beamline"]["length_cm"])
    vacuum = builder.add_tube("BeamlineVacuum", "Galactic", 0, dimensions["beamline"]["vacuum_radius_cm"], dimensions["beamline"]["length_cm"])
    target = builder.add_tube("LithiumFluorideTarget", "G4_LITHIUM_FLUORIDE", 0,
                  dimensions["target"]["radius_cm"], dimensions["target"]["thickness_cm"])
    builder.place(vacuum, target, xyz=(0, 0, dimensions["target"]["z_cm"]), rotation=(math.pi / 4, 0, 0))
    nut_size = dimensions["nut"]["size_cm"]
    rod_center_y = dimensions["cooling_rod"]["center_y_cm"]
    rod_end_clearance = rod_center_y - dimensions["cooling_rod"]["outer_radius_cm"] - nut_size[1] - nut_size[0] / 2
    rod_half_length = dimensions["beamline"]["length_cm"] / 4 - rod_end_clearance
    rod = builder.add_tube("CoolingRod", "G4_Cu", dimensions["cooling_rod"]["inner_radius_cm"], dimensions["cooling_rod"]["outer_radius_cm"], 2 * rod_half_length)
    builder.place(vacuum, rod, xyz=(0, rod_center_y, -(rod_half_length + rod_end_clearance)))
    nut = builder.add_box("BrassNut", "G4_BRASS", *nut_size)
    builder.place(vacuum, nut, xyz=(0, rod_center_y - dimensions["cooling_rod"]["outer_radius_cm"] - nut_size[1] / 2,
                           -(rod_end_clearance + nut_size[2] / 2)))
    return pipe, vacuum


def build_flanges(builder, dimensions):
    flange = builder.add_tube("Flange", "G4_Al", dimensions["flange"]["inner_radius_cm"],
                  dimensions["flange"]["outer_radius_cm"], dimensions["flange"]["length_cm"])

    return flange


def build_scintillator_barrel(builder, dimensions):
    arm_count = dimensions["scintillator"]["count"]
    if arm_count != 16:
        raise ValueError("The reference readout requires 16 scintillators")
    half_arm_angle = math.pi / arm_count
    radius = dimensions["scintillator"]["inner_radius_cm"]
    thickness = dimensions["scintillator"]["radial_thickness_cm"]
    length = dimensions["scintillator"]["length_cm"]
    wrap = dimensions["scintillator"]["wrap_thickness_cm"]
    poly = add_element(builder.solids, "polyhedra", name="ScintillatorWrapSolid", startphi=half_arm_angle,
                deltaphi=2 * math.pi + half_arm_angle, numsides=arm_count, aunit="rad", lunit="cm")
    for z in (-wrap, length + wrap):
        add_element(poly, "zplane", rmin=radius - wrap, rmax=radius + thickness + wrap, z=z)
    barrel = builder.add_volume("ScintillatorWrap", "G4_POLYVINYL_CHLORIDE", "ScintillatorWrapSolid")
    add_element(builder.solids, "trd", name="ScintillatorSolid", lunit="cm",
         x1=2 * ((radius + thickness) * math.tan(half_arm_angle) - wrap),
         x2=2 * (radius * math.tan(half_arm_angle) - wrap), y1=length, y2=length, z=thickness)
    scintillator = builder.add_volume("Scintillator", "G4_POLYSTYRENE", "ScintillatorSolid")
    return barrel, scintillator


def place_scintillators(builder, dimensions, barrel, scintillator):
    arm_count = dimensions["scintillator"]["count"]
    half_arm_angle = math.pi / arm_count
    radius = dimensions["scintillator"]["inner_radius_cm"]
    thickness = dimensions["scintillator"]["radial_thickness_cm"]
    length = dimensions["scintillator"]["length_cm"]
    for arm_index in range(arm_count):
        azimuth = arm_index * 2 * half_arm_angle
        bar_radius = radius + thickness / 2

        reference_rotation = 3.1416 / 2
        frame_rotation = (
            math.atan2(math.sin(reference_rotation) * math.cos(azimuth), math.cos(reference_rotation)),
            -math.asin(math.sin(reference_rotation) * math.sin(azimuth)),
            math.atan2(math.cos(reference_rotation) * math.sin(azimuth), math.cos(azimuth)),
        )
        builder.place(barrel, scintillator, f"Scintillator_{arm_index:02d}", arm_index,
              (-bar_radius * math.sin(azimuth), -bar_radius * math.cos(azimuth), length / 2),
              frame_rotation)


def place_delta_e_modules(builder, dimensions, world):
    half_arm_angle = math.pi / dimensions["scintillator"]["count"]
    for arm_index in range(dimensions["scintillator"]["count"]):
        azimuth = arm_index * 2 * half_arm_angle
        housing = builder.add_box(f"DeltaEHousing_{arm_index:02d}", "PLA", *dimensions["delta_e"]["housing_size_cm"])
        cavity = builder.add_box(f"DeltaECavity_{arm_index:02d}", "G4_AIR", *dimensions["delta_e"]["cavity_size_cm"])
        delta_e = builder.add_box(f"DeltaEScintillator_{arm_index:02d}", "G4_POLYSTYRENE", *dimensions["delta_e"]["size_cm"])
        builder.place(cavity, delta_e, copy=arm_index)
        builder.place(housing, cavity, copy=arm_index)
        delta_e_radius = dimensions["delta_e"]["radius_cm"]
        builder.place(world, housing, copy=arm_index, xyz=(-delta_e_radius * math.sin(azimuth), -delta_e_radius * math.cos(azimuth), 0),
              rotation=(0, 0, math.pi / 2 + azimuth))


def build_detector_geometry(config):
    dimensions = config["geometry"]
    builder = GdmlBuilder()
    build_materials(builder)
    chamber = build_tracking_chamber(builder, dimensions)
    pipe, vacuum = build_beamline(builder, dimensions)
    flange = build_flanges(builder, dimensions)
    barrel, scintillator = build_scintillator_barrel(builder, dimensions)
    length = dimensions["scintillator"]["length_cm"]

    world = builder.add_box("World", "G4_AIR", *([dimensions["world"]["size_cm"]] * 3))
    for logical in (chamber, pipe, vacuum):
        builder.place(world, logical)
    for sign, name in ((-1, "FlangeNegative"), (1, "FlangePositive")):
        builder.place(world, flange, name, xyz=(0, 0, sign * (dimensions["beamline"]["length_cm"] + dimensions["flange"]["length_cm"]) / 2))
    builder.place(world, barrel, xyz=(0, 0, -length / 2))
    place_scintillators(builder, dimensions, barrel, scintillator)
    place_delta_e_modules(builder, dimensions, world)
    return builder.finish(world)
