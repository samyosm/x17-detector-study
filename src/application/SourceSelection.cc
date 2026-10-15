#include "application/SourceSelection.hh"
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <stdexcept>

namespace {
// Date of the run (the date the activity is decayed to), written yyyy.mm.dd.
std::string RunDate(const Configuration &config) {
  std::string date = config.Get<std::string>("source.lab.activity_date", "");
  if (date.empty()) {
    const std::time_t now = std::time(nullptr);
    char buffer[16];
    std::strftime(buffer, sizeof buffer, "%Y-%m-%d", std::localtime(&now));
    date = buffer;
  }
  std::replace(date.begin(), date.end(), '-', '.');
  return date;
}
} // namespace

void SelectSource(Configuration &config, const SourceOverrides &overrides) {
  const auto seed = std::to_string(config.Get<long>("runtime.random_seed"));
  std::string id;
  if (overrides.source == "muons") {
    config.Set("source.cosmic_muons.enable", true);
    id = "0=cosmic-muons-" + RunDate(config) + "-seed" + seed;
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
    config.Set("source.lab.enable", true);
    config.Set("source.lab.id", labId);
    id = overrides.source + "=" + name + "-" + RunDate(config) + "-seed" + seed;
  }
  config.Set("simulation.id", id);
  config.Set("runtime.output_file",
             (std::filesystem::path(config.Get<std::string>("runtime.output_directory")) /
              (id + ".root")).string());
}
