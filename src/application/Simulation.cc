#include "application/Simulation.hh"
#include "G4RunManagerFactory.hh"
#include "G4Run.hh"
#include "G4VModularPhysicsList.hh"
#include "Randomize.hh"
#include "application/UiCommand.hh"
#include "application/actions/ActionInitialization.hh"
#include "geometry/DetectorConstruction.hh"
#include "physics/LabSourceDefinition.hh"
#include "physics/PhysicsList.hh"
#include "visualization/VisualizationSession.hh"
#include "output/RunFiles.hh"
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void PlaceSource(DetectorConstruction &detector, const Configuration &config, int runId,
                 int runCount) {
  const auto source = ReadLabSource(config);
  const double z = PlannedSourceZ(source.placement, config.Get<long>("runtime.random_seed"),
                                  runId, runCount);
  detector.MoveSourceCapsule({source.placement.x, source.placement.y, z});
  ExecuteUiCommand("/run/geometryModified");
}

void RunBatch(G4RunManager &manager, DetectorConstruction &detector, const Configuration &config) {
  const int events = config.Get<int>("runtime.events");
  const int checkpoint = CheckpointEventCount(config);
  const int runCount = 1 + (events - 1) / checkpoint;
  const bool movesSource = config.Get<bool>("source.lab.enable", false) &&
                           config.Get<bool>("source.lab.encapsulated", true);
  for (int completed = 0, runId = 0; completed < events; ++runId) {
    const int count = std::min(checkpoint, events - completed);
    if (movesSource)
      PlaceSource(detector, config, runId, runCount);
    ExecuteUiCommand("/run/beamOn " + std::to_string(count));
    if (manager.GetCurrentRun()->GetNumberOfEvent() != count)
      throw std::runtime_error("Run stopped before completing its checkpoint");
    CompleteRunFile(config, runId);
    completed += count;
    std::cout << "Saved " << RunOutputFile(config, runId) << " ("
              << completed << '/' << events << " events)" << std::endl;
  }
}
}

void RunSimulation(const Configuration &config) {
  if (KeepsRunRecords(config))
    G4Random::setTheSeed(config.Get<long>("runtime.random_seed"));
  if (config.Get<int>("runtime.events") <= 0)
    throw std::runtime_error("runtime.events must be positive");
  PrepareBatchOutput(config);
  auto manager = std::unique_ptr<G4RunManager>(
      G4RunManagerFactory::CreateRunManager(G4RunManagerType::MT));
  manager->SetNumberOfThreads(config.Get<int>("runtime.threads"));
  auto *detector = new DetectorConstruction(config.Path("geometry.file").string(), config);
  manager->SetUserInitialization(detector);
  manager->SetUserInitialization(CreatePhysicsList(config));
  manager->SetUserInitialization(new ActionInitialization(config));
  manager->Initialize();
  const auto mode = config.Get<std::string>("runtime.mode");
  if (mode == "batch") {
    RunBatch(*manager, *detector, config);
    MergeCompletedRun(config);
    if (config.Get<bool>("readout.raw_hits.enable", false))
      EmbedRunInfo(config);
    std::cout << "Completed output: " << config.Get<std::string>("runtime.output_file") << std::endl;
  }
  else if (mode == "visualization")
    OpenDetectorViewer(config);
  else
    throw std::runtime_error("Unknown runtime mode: " + mode);
}
