#pragma once
#include "G4UserEventAction.hh"
#include "configuration/Configuration.hh"
#include "readout/EventReadout.hh"
#include <memory>

class CosmicReadout;
class G4Step;
class PrimaryGeneratorAction;

class EventAction final : public G4UserEventAction {
public:
    EventAction(const PrimaryGeneratorAction* source, int progressInterval,
                const Configuration* cosmic = nullptr);
    ~EventAction() override;
    void RecordSensitiveEnergyDeposit(const G4Step* step);
    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;
    void RecordScintillatorEnergy(int bar, double energy);
    void RecordDetectedPhoton(int channel, double time, double energy);
private:
    std::unique_ptr<CosmicReadout> cosmic_;
    EventReadout readout_;
    const PrimaryGeneratorAction* source_;
    int progressInterval_;
    bool recordSteps_;
};
