#include "application/actions/RunAction.hh"
#include "output/RootOutput.hh"
#include "output/RunFiles.hh"

#include "G4AnalysisManager.hh"

#include <utility>

RunAction::RunAction(const Configuration& config)
    : config_(config) {
    DefineRootTrees();
}

void RunAction::BeginOfRunAction(const G4Run*) {
    if (IsMaster()) PrepareRunFiles(config_);
    G4AnalysisManager::Instance()->OpenFile(config_.Get<std::string>("runtime.output_file"));
}

void RunAction::EndOfRunAction(const G4Run*) {
    auto* analysis = G4AnalysisManager::Instance();
    analysis->Write();
    analysis->CloseFile();
}
