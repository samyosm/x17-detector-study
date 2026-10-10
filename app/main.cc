#include "application/Simulation.hh"
#include "application/SourceSelection.hh"
#include "configuration/Configuration.hh"
#include <exception>
#include <iostream>
#include <stdexcept>

namespace {
struct Arguments {
  std::filesystem::path configuration = CONFIG_PATH;
  std::string source;
};

Arguments ReadArguments(int argumentCount, char **arguments) {
  Arguments result;
  for (int index = 1; index < argumentCount; index += 2) {
    const std::string name = arguments[index];
    if (index + 1 == argumentCount)
      throw std::invalid_argument("Missing value for " + name);
    if (name == "--config")
      result.configuration = arguments[index + 1];
    else if (name == "--source")
      result.source = arguments[index + 1];
    else
      throw std::invalid_argument(
          "Usage: simulate_detection [--config path] [--source muons|LAB_ID]");
  }
  return result;
}
} // namespace

int main(int argumentCount, char **arguments) {
  try {
    const auto selected = ReadArguments(argumentCount, arguments);
    Configuration config(selected.configuration);
    if (!selected.source.empty())
      SelectSource(config, selected.source);
    RunSimulation(config);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "Error: " << error.what() << '\n';
    return 1;
  }
}
