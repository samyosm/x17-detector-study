#include "readout/LightMap.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SystemOfUnits.hh"
#include "G4Trd.hh"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

double BarShape::HalfWidth(double z) const {
  const double fraction = (z + halfDepth) / (2 * halfDepth);
  return halfWidthAtBack + (halfWidthAtFront - halfWidthAtBack) * std::clamp(fraction, 0.0, 1.0);
}

BarShape FindBarShape() {
  auto *volume = G4LogicalVolumeStore::GetInstance()->GetVolume("Scintillator", false);
  const auto *trapezoid = volume ? dynamic_cast<const G4Trd *>(volume->GetSolid()) : nullptr;
  if (!trapezoid)
    throw std::runtime_error("The Scintillator volume must be a G4Trd to use the light map");
  return {trapezoid->GetXHalfLength1(), trapezoid->GetXHalfLength2(), trapezoid->GetYHalfLength1(),
          trapezoid->GetZHalfLength()};
}

LightMap::LightMap(int bandsY, int bandsDepth, int bandsWidth)
    : ny_(bandsY), nz_(bandsDepth), nx_(bandsWidth) {
  if (ny_ < 1 || nz_ < 1 || nx_ < 1)
    throw std::runtime_error("Light map needs at least one band along each axis");
  for (auto &values : collection_)
    values.assign(CellCount(), 0.0f);
}

G4ThreeVector LightMap::LocalPoint(const BarShape &shape, double fy, double fz, double fx) {
  const double z = (2 * fz - 1) * shape.halfDepth;
  return {(2 * fx - 1) * shape.HalfWidth(z), (2 * fy - 1) * shape.halfLength, z};
}

std::array<double, 2> LightMap::Collection(const BarShape &shape, const G4ThreeVector &local) const {
  const double fz = std::clamp((local.z() + shape.halfDepth) / (2 * shape.halfDepth), 0.0, 1.0);
  const double fy = std::clamp((local.y() + shape.halfLength) / (2 * shape.halfLength), 0.0, 1.0);
  const double fx = std::clamp((local.x() / shape.HalfWidth(local.z()) + 1) / 2, 0.0, 1.0);
  struct Axis {
    int low, high;
    double weight;
  };
  auto locate = [](double fraction, int bands) {
    const double position = std::clamp(fraction * bands - 0.5, 0.0, bands - 1.0);
    const int low = static_cast<int>(std::floor(position));
    return Axis{low, std::min(low + 1, bands - 1), position - low};
  };
  const Axis y = locate(fy, ny_), z = locate(fz, nz_), x = locate(fx, nx_);
  std::array<double, 2> result{0.0, 0.0};
  for (int cy = 0; cy < 2; ++cy)
    for (int cz = 0; cz < 2; ++cz)
      for (int cx = 0; cx < 2; ++cx) {
        const double weight = (cy ? y.weight : 1 - y.weight) * (cz ? z.weight : 1 - z.weight) *
                              (cx ? x.weight : 1 - x.weight);
        const auto cell = CellIndex(cy ? y.high : y.low, cz ? z.high : z.low, cx ? x.high : x.low);
        for (int end = 0; end < 2; ++end)
          result[end] += weight * collection_[end][cell];
      }
  return result;
}

std::filesystem::path LightMap::File(const Configuration &config) {
  return config.Path("optics.light_map.file");
}

std::map<std::string, std::string> LightMap::Describe(const Configuration &config) {
  auto text = [](double value) {
    std::ostringstream stream;
    stream.precision(10);
    stream << value;
    return stream.str();
  };
  return {{"reflectivity", text(config.Get<double>("optics.wrap.reflectivity"))},
          {"absorption_length_cm", text(config.Get<double>("optics.scintillator.absorption_length_cm"))},
          {"refractive_index", text(config.Get<double>("optics.scintillator.refractive_index"))},
          {"bands_y", std::to_string(config.Get<int>("optics.light_map.bands_y"))},
          {"bands_depth", std::to_string(config.Get<int>("optics.light_map.bands_depth"))},
          {"bands_width", std::to_string(config.Get<int>("optics.light_map.bands_width"))}};
}

void LightMap::Save(const std::filesystem::path &file, const std::map<std::string, std::string> &metadata) const {
  std::ofstream output(file);
  output.precision(9);
  for (const auto &[key, value] : metadata)
    output << "# " << key << " = " << value << '\n';
  output << "# columns: iy iz ix D_end U_end\n";
  for (int iy = 0; iy < ny_; ++iy)
    for (int iz = 0; iz < nz_; ++iz)
      for (int ix = 0; ix < nx_; ++ix) {
        const auto cell = CellIndex(iy, iz, ix);
        output << iy << ' ' << iz << ' ' << ix << ' ' << collection_[0][cell] << ' ' << collection_[1][cell] << '\n';
      }
  if (!output)
    throw std::runtime_error("Could not write light map: " + file.string());
}

const LightMap &LightMap::Shared(const Configuration &config) {
  static const LightMap map = [&config] {
    const auto file = File(config);
    std::ifstream input(file);
    if (!input)
      throw std::runtime_error("Light map not found: " + file.string() +
                               " (build it with: simulate_detection --config <config> --build-light-map)");
    std::map<std::string, std::string> stored;
    std::string line;
    std::vector<std::array<double, 5>> rows;
    while (std::getline(input, line)) {
      if (line.rfind("# columns", 0) == 0)
        continue;
      if (line.rfind("# ", 0) == 0) {
        const auto split = line.find(" = ");
        if (split != std::string::npos)
          stored[line.substr(2, split - 2)] = line.substr(split + 3);
        continue;
      }
      std::istringstream row(line);
      std::array<double, 5> values;
      if (row >> values[0] >> values[1] >> values[2] >> values[3] >> values[4])
        rows.push_back(values);
    }
    for (const auto &[key, value] : Describe(config)) {
      const auto found = stored.find(key);
      if (found == stored.end() || std::abs(std::stod(found->second) - std::stod(value)) > 1e-6 * (1 + std::abs(std::stod(value))))
        throw std::runtime_error("Light map " + file.string() + " was built with a different " + key +
                                 "; rebuild it with --build-light-map");
    }
    const auto shape = FindBarShape();
    const std::map<std::string, double> geometry = {{"bar_half_length_cm", shape.halfLength / cm},
                                                    {"bar_half_depth_cm", shape.halfDepth / cm},
                                                    {"bar_half_width_back_cm", shape.halfWidthAtBack / cm},
                                                    {"bar_half_width_front_cm", shape.halfWidthAtFront / cm}};
    for (const auto &[key, value] : geometry) {
      const auto found = stored.find(key);
      if (found == stored.end() || std::abs(std::stod(found->second) - value) > 1e-6 * (1 + std::abs(value)))
        throw std::runtime_error("Light map " + file.string() + " was built for a different bar shape (" + key +
                                 "); rebuild it with --build-light-map");
    }
    LightMap result(std::stoi(stored.at("bands_y")), std::stoi(stored.at("bands_depth")),
                    std::stoi(stored.at("bands_width")));
    if (rows.size() != result.CellCount())
      throw std::runtime_error("Light map " + file.string() + " is incomplete");
    for (const auto &row : rows)
      result.Set(result.CellIndex(static_cast<int>(row[0]), static_cast<int>(row[1]), static_cast<int>(row[2])),
                 row[3], row[4]);
    return result;
  }();
  return map;
}
