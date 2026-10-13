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
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <set>
#include <cstdint>
#include <iostream>

namespace {
constexpr double photonTolerance = 1.0 * ns;
constexpr double adcMaximum = 32767.0; // signed 16-bit ADC word
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
    if (config.Get<bool>("readout.raw_hits.enable", false)) {
        barHits_ = std::make_unique<BarHitReadout>(config);
        adcPerPhotoelectron_ = config.Get<double>("readout.vf48.adc_per_photoelectron");
        eventPeriod_ = config.Get<double>("readout.vf48.event_period_ns") * ns;
        timeResolution_ = config.Get<double>("readout.vf48.time_resolution_ns") * ns;
        clock_ = std::make_unique<EventClock>();
    }
    const bool cosmic = !barHits_ && config.Get<bool>("source.cosmic_muons.enable", false);
    if (cosmic) cosmic_ = std::make_unique<CosmicReadout>(config);
    recordSteps_ = cosmic && config.Get<bool>("readout.record_steps");
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
        photons_.clear();
    }
}

void EventAction::RecordScintillatorEnergy(int bar, double energy) {
    if (bar >= 1 && bar <= detector::armCount)
        readout_.depositedEnergy[bar - 1] += energy;
}

void EventAction::RecordDetectedPhoton(int channel, double time, double energy) {
    if (channel < 1 || channel > detector::sensorCount) return;
    if (barHits_) {
        photons_.push_back({channel - 1, clock_->SinceDecay(time)});
        return;
    }
    const int sensorIndex = channel - 1;
    if (readout_.photonCount[sensorIndex] == 0 || time / ns < readout_.firstPhotonTimeNs[sensorIndex])
        readout_.firstPhotonTimeNs[sensorIndex] = time / ns;
    ++readout_.photonCount[sensorIndex];
    WritePhotonHit(readout_.eventId, channel, time, energy);
}

void EventAction::EndOfEventAction(const G4Event* event) {
    if (!barHits_) {
        const int mode = source_->GetModeId();
        WriteEvent(readout_, mode, mode == 3 ? PairObservables{}
                                           : CalculatePrimaryPairObservables(*event));
    }
    if (cosmic_) {
        WriteArmDeposits(readout_.eventId, cosmic_->Deposits());
        WriteCosmicEvent(readout_.eventId, *event, CalculateCosmicObservables(cosmic_->Deposits(), cosmic_->Thresholds()));
    }
    if (barHits_) {
        const auto hits = barHits_->Hits();
        std::vector<std::array<int, 2>> counts(hits.size(), {0, 0});
        for (const auto& photon : photons_) {
            const int bar = photon.sensor / detector::sensorsPerArm + 1;
            int owner = -1;
            for (std::size_t i = 0; i < hits.size(); ++i) {
                if (hits[i].bar != bar) continue;
                if (owner < 0 || hits[i].start - photonTolerance <= photon.time) owner = static_cast<int>(i);
            }
            if (owner >= 0) ++counts[owner][photon.sensor % detector::sensorsPerArm];
        }
        const double eventTime = readout_.eventId * eventPeriod_;
        std::set<long long> usedSlots;
        for (std::size_t i = 0; i < hits.size(); ++i) {
            long long slot = std::llround(hits[i].time / timeResolution_);
            while (!usedSlots.insert(slot).second) ++slot;
            std::array<int, 2> adc;
            for (int end = 0; end < 2; ++end)
                adc[end] = static_cast<int>(std::min(adcMaximum, std::round(counts[i][end] * adcPerPhotoelectron_)));
            WriteRawHit(readout_.eventId, static_cast<int>(i), hits[i], eventTime + slot * timeResolution_, adc);
        }
        WriteTruth(readout_.eventId, source_->GetLabId(), *event);
    }
    ReportProgress(progressInterval_);
}
