#include "application/actions/EventAction.hh"
#include "application/actions/PrimaryGeneratorAction.hh"
#include "output/RunFiles.hh"
#include "readout/BarHitReadout.hh"
#include "readout/CosmicReadout.hh"
#include "readout/EventClock.hh"
#include "output/RootOutput.hh"
#include "output/CosmicWriter.hh"
#include "physics/CosmicObservables.hh"
#include "physics/PairObservables.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include <atomic>
#include <cstdint>
#include <iostream>

namespace {
void ReportProgress(int interval) {
    static std::atomic<std::uint64_t> completedEvents{0};
    const auto completed = completedEvents.fetch_add(1, std::memory_order_relaxed) + 1;
    if (interval > 0 && completed % interval == 0)
        std::cout << completed << " events processed" << std::endl;
}
}

EventAction::EventAction(const PrimaryGeneratorAction* source, const Configuration& config)
    : source_(source), progressInterval_(config.Get<int>("runtime.progress_interval")),
      checkpointEvents_(UsesCheckpoints(config) ? CheckpointEventCount(config) : 0) {
    const bool cosmic = config.Get<bool>("source.cosmic_muons.enable", false);
    if (cosmic) cosmic_ = std::make_unique<CosmicReadout>(config);
    recordSteps_ = cosmic && config.Get<bool>("readout.record_steps");
    if (config.Get<bool>("readout.raw_hits.enable", false)) {
        barHits_ = std::make_unique<BarHitReadout>(config);
        clock_ = std::make_unique<EventClock>();
    }
}

void EventAction::TrackStep(const G4Step* step) {
    if (clock_) clock_->RegisterSecondaries(*step);
}

EventAction::~EventAction() = default;

void EventAction::RecordSensitiveEnergyDeposit(const G4Step* step, detector::Layer layer) {
    if (barHits_ && layer == detector::scintillator)
        barHits_->RecordDeposit(*step, step->GetPreStepPoint()->GetTouchableHandle()->GetCopyNumber(),
                                clock_->StepTime(*step));
    if (!cosmic_) return;
    const auto address = cosmic_->RecordDeposit(*step, layer);
    if (recordSteps_ && address)
        WriteDetectorStep(readout_.eventId, address->layer, address->armIndex, *step);
}

void EventAction::BeginOfEventAction(const G4Event* event) {
    const int offset = checkpointEvents_ * G4RunManager::GetRunManager()->GetCurrentRun()->GetRunID();
    readout_ = EventReadout(offset + event->GetEventID());
    if (cosmic_) cosmic_->Reset();
    if (barHits_) {
        barHits_->Reset();
        clock_->Reset();
    }
}

void EventAction::RecordScintillatorEnergy(int bar, double energy) {
    if (bar >= 1 && bar <= detector::armCount)
        readout_.depositedEnergy[bar - 1] += energy;
}

void EventAction::RecordDetectedPhoton(int channel, double time, double energy) {
    if (channel < 1 || channel > detector::sensorCount) return;
    const int sensorIndex = channel - 1;
    if (readout_.photonCount[sensorIndex] == 0 || time / ns < readout_.firstPhotonTimeNs[sensorIndex])
        readout_.firstPhotonTimeNs[sensorIndex] = time / ns;
    ++readout_.photonCount[sensorIndex];
    WritePhotonHit(readout_.eventId, channel, time, energy);
}

void EventAction::EndOfEventAction(const G4Event* event) {
    const int mode = source_->GetModeId();
    WriteEvent(readout_, mode, mode == 3 ? PairObservables{}
                                       : CalculatePrimaryPairObservables(*event));
    if (cosmic_) {
        WriteArmDeposits(readout_.eventId, cosmic_->Deposits());
        WriteCosmicEvent(readout_.eventId, *event, CalculateCosmicObservables(cosmic_->Deposits(), cosmic_->Thresholds()));
    }
    if (barHits_) {
        for (const auto& hit : barHits_->Hits()) WriteRawHit(readout_.eventId, hit);
        WriteTruth(readout_.eventId, source_->GetLabId(), *event);
    }
    ReportProgress(progressInterval_);
}
