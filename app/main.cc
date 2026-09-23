#include "application/Simulation.hh"
#include "configuration/Configuration.hh"
#include <exception>
#include <iostream>
#include <stdexcept>

namespace {
std::filesystem::path ReadConfigurationPath(int argumentCount,
                                            char **arguments) {
  if (argumentCount == 1)
    return CONFIG_PATH;
  if (argumentCount == 3 && std::string(arguments[1]) == "--config")
    return arguments[2];
  throw std::invalid_argument("Usage: simulate_detection [--config path]");
}
} // namespace

int main(int argumentCount, char **arguments) {
  try {
    RunSimulation(
        Configuration(ReadConfigurationPath(argumentCount, arguments)));
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "Error: " << error.what() << '\n';
    return 1;
  }
}
