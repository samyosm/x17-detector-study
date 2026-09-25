#include "application/actions/SteppingAction.hh"
#include "application/actions/EventAction.hh"
#include "geometry/DetectorLayout.hh"
#include "readout/OpticalSensor.hh"
#include "G4LogicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4OpticalPhoton.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "Randomize.hh"

SteppingAction::SteppingAction(EventAction* eventAction, double quantumEfficiency)
    : eventAction_(eventAction), quantumEfficiency_(quantumEfficiency) {
    for (const auto* volume : *G4LogicalVolumeStore::GetInstance()) {
        const auto index = static_cast<std::size_t>(volume->GetInstanceID());
        if (index >= layers_.size()) layers_.resize(index + 1, detector::outside);
        layers_[index] = detector::FindSensitiveLayer(volume->GetName());
    }
}

detector::Layer SteppingAction::FindLayer(const G4LogicalVolume* volume) const {
    const auto index = static_cast<std::size_t>(volume->GetInstanceID());
    if (index < layers_.size()) return layers_[index];
    return detector::FindSensitiveLayer(volume->GetName());
}

void SteppingAction::UserSteppingAction(const G4Step* step) {
    const auto& touchable = step->GetPreStepPoint()->GetTouchableHandle();
    const auto* volume = touchable->GetVolume();
    if (!volume) return;
    const auto layer = FindLayer(volume->GetLogicalVolume());
    if (layer == detector::outside) return;
    eventAction_->RecordSensitiveEnergyDeposit(step, layer);
    if (layer != detector::scintillator) return;
    auto* track = step->GetTrack();
    if (track->GetDefinition() != G4OpticalPhoton::Definition()) {
        eventAction_->RecordScintillatorEnergy(touchable->GetCopyNumber() + 1, step->GetTotalEnergyDeposit());
        return;
    }
    const int sensor = FindCrossedEndSensor(*step);
    if (sensor < 0) return;
    if (G4UniformRand() < quantumEfficiency_)
        eventAction_->RecordDetectedPhoton(sensor + 1, step->GetPostStepPoint()->GetGlobalTime(),
                                  step->GetPreStepPoint()->GetKineticEnergy());
    track->SetTrackStatus(fStopAndKill);
}
