#include "application/actions/RunAction.hh"
#include "output/RootOutput.hh"
#include "output/RunFiles.hh"
#include "physics/CosmicMuonSpectrum.hh"

#include "G4AnalysisManager.hh"

#include "G4Run.hh"
#include <stdexcept>

RunAction::RunAction(const Configuration& config)
    : config_(config) {
    DefineRootTrees();
}

void RunAction::BeginOfRunAction(const G4Run* run) {
    if (IsMaster()) {
        if (!cosmicRateHz_ && config_.Get<bool>("source.cosmic_muons.enable", false)) {
            const double sideCm = config_.Get<double>("source.cosmic_muons.plane.side_cm");
            cosmicRateHz_ = sideCm * sideCm *
                CosmicMuonSpectrum(config_).HorizontalFluxPerCm2Second();
        }
        PrepareRunFiles(config_, run->GetRunID(), cosmicRateHz_.value_or(0));
    }
    const auto path = PendingRunOutputFile(config_, run->GetRunID()).string();
    if (!G4AnalysisManager::Instance()->OpenFile(path))
        throw std::runtime_error("Could not open ROOT output: " + path);
}

void RunAction::EndOfRunAction(const G4Run*) {
    auto* analysis = G4AnalysisManager::Instance();
    const bool written = analysis->Write();
    const bool closed = analysis->CloseFile();
    if (!written || !closed)
        throw std::runtime_error("Could not finalize ROOT output; checkpoint remains incomplete");
}
