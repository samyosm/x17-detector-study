#include "application/SourceSelection.hh"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <stdexcept>

namespace {
std::string Lowercase(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(),
                 [](unsigned char letter) { return std::tolower(letter); });
  return text;
}

std::string Number(double value) {
  std::ostringstream text;
  text << value;
  return text.str();
}

std::string PlacementLabel(const Configuration &config, bool encapsulated) {
  std::string label = "_x" + Number(config.Get<double>("placement.x_cm")) + "_y" +
                      Number(config.Get<double>("placement.y_cm"));
  if (config.Get<std::string>("placement.z_mode") == "uniform")
    label += "_z" + Number(config.Get<double>("placement.z_min_cm")) + "to" +
             Number(config.Get<double>("placement.z_max_cm"));
  else
    label += "_z" + Number(config.Get<double>("placement.z_cm"));
  if (encapsulated) {
    const auto direction = config.Vector("placement.orientation");
    label += "_o" + Number(direction[0]) + "_" + Number(direction[1]) + "_" + Number(direction[2]);
  }
  return label;
}
} // namespace

void SelectSource(Configuration &config, const SourceOverrides &overrides) {
  const auto seed = std::to_string(config.Get<long>("runtime.random_seed"));
  std::string id;
  if (overrides.source == "muons") {
    config.Set("source.cosmic_muons.enable", true);
    id = "muons_seed" + seed;
  } else {
    const auto labId = std::stoi(overrides.source);
    const auto name = config.Get<std::string>("sources." + overrides.source + ".name");
    if (overrides.x)
      config.Set("placement.x_cm", *overrides.x);
    if (overrides.y)
      config.Set("placement.y_cm", *overrides.y);
    if (overrides.z) {
      config.Set("placement.z_mode", std::string("fixed"));
      config.Set("placement.z_cm", *overrides.z);
    }
    if (overrides.bare)
      config.Set("source.lab.encapsulated", false);
    const bool encapsulated = config.Get<bool>("source.lab.encapsulated", true);
    config.Set("source.lab.enable", true);
    config.Set("source.lab.id", labId);
    id = "lab" + overrides.source + "_" + Lowercase(name) + (encapsulated ? "_capsule" : "_bare") +
         PlacementLabel(config, encapsulated) + "_seed" + seed;
  }
  config.Set("simulation.id", id);
  config.Set("runtime.output_file",
             (std::filesystem::path(config.Get<std::string>("runtime.output_directory")) /
              (id + ".root")).string());
}
