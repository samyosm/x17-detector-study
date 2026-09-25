import sys
from pathlib import Path
import tomllib
import xml.etree.ElementTree as ET

project = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(project / "scripts"))
from geometry.detector import build_detector_geometry

with (project / "config/geometry.toml").open("rb") as stream:
    config = tomllib.load(stream)
actual = ET.tostring(build_detector_geometry(config).getroot())
expected = ET.tostring(ET.parse(project / "geometry/world.gdml").getroot())
if actual != expected:
    raise AssertionError("Generated detector differs from the checked-in GDML")
print("Detector geometry matches the checked-in GDML.")
