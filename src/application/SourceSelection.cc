#include "application/SourceSelection.hh"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>

namespace {
std::string Lowercase(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(),
                 [](unsigned char letter) { return std::tolower(letter); });
  return text;
}
} // namespace

void SelectSource(Configuration &config, const std::string &source) {
  const auto seed = std::to_string(config.Get<long>("runtime.random_seed"));
  std::string id;
  if (source == "muons") {
    config.Set("source.cosmic_muons.enable", true);
    id = "muons_seed" + seed;
  } else {
    const auto labId = std::stoi(source);
    const auto name = config.Get<std::string>("sources." + source + ".name");
    config.Set("source.lab.enable", true);
    config.Set("source.lab.id", labId);
    id = "lab" + source + "_" + Lowercase(name) + "_seed" + seed;
  }
  config.Set("simulation.id", id);
  config.Set("runtime.output_file",
             (std::filesystem::path(config.Get<std::string>("runtime.output_directory")) /
              (id + ".root")).string());
}
