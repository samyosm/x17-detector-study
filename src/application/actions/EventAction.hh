#pragma once
#include "G4UserEventAction.hh"
#include "configuration/Configuration.hh"
#include "readout/EventReadout.hh"
#include <memory>

class BarHitReadout;
class CosmicReadout;
class EventClock;
class G4Step;
class PrimaryGeneratorAction;

class EventAction final : public G4UserEventAction {
public:
    EventAction(const PrimaryGeneratorAction* source, const Configuration& config);
    ~EventAction() override;
    void TrackStep(const G4Step* step);
    void RecordSensitiveEnergyDeposit(const G4Step* step, detector::Layer layer);
    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;
    void RecordScintillatorEnergy(int bar, double energy);
    void RecordDetectedPhoton(int channel, double time, double energy);
private:
    std::unique_ptr<CosmicReadout> cosmic_;
    std::unique_ptr<BarHitReadout> barHits_;
    std::unique_ptr<EventClock> clock_;
    EventReadout readout_;
    const PrimaryGeneratorAction* source_;
    int progressInterval_;
    int checkpointEvents_;
    bool recordSteps_;
};
