#include "application/Simulation.hh"
#include "application/SourceSelection.hh"
#include "configuration/Configuration.hh"
#include <exception>
#include <iostream>
#include <stdexcept>

namespace {
struct Arguments {
  std::filesystem::path configuration = CONFIG_PATH;
  SourceOverrides source;
};

Arguments ReadArguments(int argumentCount, char **arguments) {
  Arguments result;
  for (int index = 1; index < argumentCount; ++index) {
    const std::string name = arguments[index];
    if (name == "--bare") {
      result.source.bare = true;
      continue;
    }
    if (index + 1 == argumentCount)
      throw std::invalid_argument("Missing value for " + name);
    const std::string value = arguments[++index];
    if (name == "--config")
      result.configuration = value;
    else if (name == "--source")
      result.source.source = value;
    else if (name == "--x")
      result.source.x = std::stod(value);
    else if (name == "--y")
      result.source.y = std::stod(value);
    else if (name == "--z")
      result.source.z = std::stod(value);
    else
      throw std::invalid_argument(
          "Usage: simulate_detection [--config path] [--source muons|LAB_ID] "
          "[--bare] [--x cm] [--y cm] [--z cm]");
  }
  return result;
}
} // namespace

int main(int argumentCount, char **arguments) {
  try {
    const auto selected = ReadArguments(argumentCount, arguments);
    Configuration config(selected.configuration);
    if (!selected.source.source.empty())
      SelectSource(config, selected.source);
    RunSimulation(config);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "Error: " << error.what() << '\n';
    return 1;
  }
}
