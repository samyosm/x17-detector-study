#include "visualization/VisualizationSession.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"
#include "application/UiCommand.hh"
#include "visualization/CosmicSourceVisualization.hh"
#include <memory>

void OpenDetectorViewer(const Configuration &config) {
  std::unique_ptr<CosmicSourceVisualization> sourcePlane;
  if (config.Get<bool>("source.cosmic_muons.enable", false))
    sourcePlane = std::make_unique<CosmicSourceVisualization>(config);
  G4VisExecutive visualization;
  visualization.Initialize();
  if (sourcePlane)
    visualization.RegisterRunDurationUserVisAction(
        "CosmicSource", sourcePlane.get(), sourcePlane->Extent());
  char name[] = "simulate_detection";
  char *arguments[] = {name, nullptr};
  G4UIExecutive session(1, arguments);
  ExecuteUiCommand("/control/execute " +
                   config.Path("visualization.macro").string());
  if (sourcePlane) {
    ExecuteUiCommand("/vis/scene/add/userAction CosmicSource");
    ExecuteUiCommand("/vis/viewer/flush");
  }
  session.SessionStart();
}
