from pathlib import Path
import tomllib
from geometry.detector import build_detector_geometry


def main():
    configuration_path = Path(__file__).resolve().parents[1] / "config/geometry.toml"
    with configuration_path.open("rb") as stream:
        configuration = tomllib.load(stream)
    output_path = configuration_path.parent / configuration["geometry"]["file"]
    output_path.parent.mkdir(parents=True, exist_ok=True)
    build_detector_geometry(configuration).write(output_path, encoding="unicode", xml_declaration=True)


if __name__ == "__main__":
    main()
