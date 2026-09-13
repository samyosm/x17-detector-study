#include "SteppingAction.hh"
#include "EventAction.hh"

#include "G4LogicalVolume.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4VPhysicalVolume.hh"

SteppingAction::SteppingAction(EventAction *eventAction)
    : eventAction_(eventAction) {}

void SteppingAction::UserSteppingAction(const G4Step *step) {
  const auto &touchable = step->GetPreStepPoint()->GetTouchableHandle();
  const auto *volume = touchable->GetVolume();
  if (volume && volume->GetLogicalVolume()->GetName() == "Scintillator") {
    eventAction_->AddBarEnergy(touchable->GetCopyNumber(),
                               step->GetTotalEnergyDeposit());
  }
}
