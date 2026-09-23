#include "application/Simulation.hh"
#include "G4RunManagerFactory.hh"
#include "G4VModularPhysicsList.hh"
#include "Randomize.hh"
#include "application/UiCommand.hh"
#include "application/actions/ActionInitialization.hh"
#include "geometry/DetectorConstruction.hh"
#include "physics/PhysicsList.hh"
#include "visualization/VisualizationSession.hh"
#include <memory>
#include <stdexcept>

void RunSimulation(const Configuration &config) {
  if (config.Get<bool>("source.cosmic_muons.enable", false))
    G4Random::setTheSeed(config.Get<long>("runtime.random_seed"));
  auto manager = std::unique_ptr<G4RunManager>(
      G4RunManagerFactory::CreateRunManager(G4RunManagerType::MT));
  manager->SetNumberOfThreads(config.Get<int>("runtime.threads"));
  manager->SetUserInitialization(
      new DetectorConstruction(config.Path("geometry.file").string(), config));
  manager->SetUserInitialization(CreatePhysicsList(config));
  manager->SetUserInitialization(new ActionInitialization(config));
  manager->Initialize();
  const auto mode = config.Get<std::string>("runtime.mode");
  if (mode == "batch")
    ExecuteUiCommand("/run/beamOn " +
                     std::to_string(config.Get<int>("runtime.events")));
  else if (mode == "visualization")
    OpenDetectorViewer(config);
  else
    throw std::runtime_error("Unknown runtime mode: " + mode);
}
