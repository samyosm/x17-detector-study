#include "application/actions/SteppingAction.hh"
#include "application/actions/EventAction.hh"
#include "geometry/DetectorLayout.hh"
#include "readout/OpticalSensor.hh"
#include "G4LogicalVolume.hh"
#include "G4OpticalPhoton.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "Randomize.hh"

SteppingAction::SteppingAction(EventAction* eventAction, double quantumEfficiency)
    : eventAction_(eventAction), quantumEfficiency_(quantumEfficiency) {}

void SteppingAction::UserSteppingAction(const G4Step* step) {
    eventAction_->RecordSensitiveEnergyDeposit(step);
    const auto& touchable = step->GetPreStepPoint()->GetTouchableHandle();
    const auto* volume = touchable->GetVolume();
    if (!volume || detector::FindSensitiveLayer(volume->GetLogicalVolume()->GetName()) != detector::scintillator)
        return;
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
