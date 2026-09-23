import xml.etree.ElementTree as ET
import math


def add_element(parent, tag, **attributes):
    return ET.SubElement(parent, tag, {key: str(value) for key, value in attributes.items()})


class GdmlBuilder:
    def __init__(self):
        self.root = ET.Element("gdml")
        add_element(self.root, "define")
        self.materials = add_element(self.root, "materials")
        self.solids = add_element(self.root, "solids")
        self.structure = add_element(self.root, "structure")

    def add_material(self, name, density, components, atoms=False):
        material = add_element(self.materials, "material", name=name)
        add_element(material, "D", value=density, unit="g/cm3")
        for reference, amount in components:
            add_element(material, "composite" if atoms else "fraction", n=amount, ref=reference)
        return material

    def add_volume(self, name, material, solid):
        volume = add_element(self.structure, "volume", name=name)
        add_element(volume, "materialref", ref=material)
        add_element(volume, "solidref", ref=solid)
        return volume

    def add_box(self, name, material, x, y, z):
        add_element(self.solids, "box", name=name + "Solid", x=x, y=y, z=z, lunit="cm")
        return self.add_volume(name, material, name + "Solid")

    def add_tube(self, name, material, inner_radius, outer_radius, length):
        add_element(self.solids, "tube", name=name + "Solid", rmin=inner_radius, rmax=outer_radius,
                    z=length, startphi=0, deltaphi=2 * math.pi, aunit="rad", lunit="cm")
        return self.add_volume(name, material, name + "Solid")

    def place(self, parent, volume, name=None, copy=0, xyz=(0, 0, 0), rotation=(0, 0, 0)):
        name = name or volume.get("name")
        placement = add_element(parent, "physvol", name=name, copynumber=copy)
        add_element(placement, "volumeref", ref=volume.get("name"))
        add_element(placement, "position", name=name + "Position", unit="cm",
                    **dict(zip(("x", "y", "z"), xyz)))
        add_element(placement, "rotation", name=name + "Rotation", unit="rad",
                    **dict(zip(("x", "y", "z"), rotation)))

    def finish(self, world):
        volumes = {volume.get("name"): volume for volume in self.structure}
        ordered = []
        visited = set()

        def visit_daughters_first(volume):
            if volume.get("name") in visited:
                return
            for reference in volume.findall("physvol/volumeref"):
                visit_daughters_first(volumes[reference.get("ref")])
            visited.add(volume.get("name"))
            ordered.append(volume)

        visit_daughters_first(world)
        self.structure[:] = ordered
        setup = add_element(self.root, "setup", name="Default", version="1.0")
        add_element(setup, "world", ref="World")
        ET.indent(self.root, space="  ")
        return ET.ElementTree(self.root)
