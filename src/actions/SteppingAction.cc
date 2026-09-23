#include "SteppingAction.hh"
#include "EventAction.hh"

#include "G4LogicalVolume.hh"
#include "G4OpticalPhoton.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4SystemOfUnits.hh"
#include "G4TouchableHistory.hh"
#include "G4Track.hh"
#include "G4Trd.hh"
#include "G4VPhysicalVolume.hh"
#include "Randomize.hh"

#include <cmath>

SteppingAction::SteppingAction(EventAction *eventAction,
                               double quantumEfficiency)
    : eventAction_(eventAction), quantumEfficiency_(quantumEfficiency) {}

void SteppingAction::UserSteppingAction(const G4Step *step) {
  const auto *pre = step->GetPreStepPoint();
  const auto &touchable = pre->GetTouchableHandle();
  const auto *volume = touchable->GetVolume();
  if (!volume || volume->GetLogicalVolume()->GetName() != "Scintillator")
    return;
  auto *track = step->GetTrack();
  const int bar = touchable->GetCopyNumber() + 1; // official copies are 0..15
  if (track->GetParticleDefinition() != G4OpticalPhoton::Definition()) {
    eventAction_->AddBarEnergy(bar, step->GetTotalEnergyDeposit());
    return;
  }
  const auto *post = step->GetPostStepPoint();
  if (post->GetStepStatus() != fGeomBoundary)
    return;
  const auto *solid =
      dynamic_cast<const G4Trd *>(volume->GetLogicalVolume()->GetSolid());
  if (!solid)
    return;
  const auto local = touchable->GetHistory()->GetTopTransform().TransformPoint(
      post->GetPosition());
  // The official trapezoids have their long axis along local y. Its positive
  // end points toward global -z, including the reference's small rotation tilt.
  if (std::abs(std::abs(local.y()) - solid->GetYHalfLength1()) > 1e-6 * mm)
    return;

  const int channel =
      2 * bar - (local.y() > 0 ? 1 : 0); // D=-z, U=+z extension convention
  // Virtual absorbing photocathode: count arrival at the scintillator end,
  // including photons killed by the PVC boundary's missing refractive index.
  if (G4UniformRand() < quantumEfficiency_) {
    eventAction_->AddPmtPhoton(channel, post->GetGlobalTime(),
                               pre->GetKineticEnergy());
  }
  track->SetTrackStatus(fStopAndKill);
}
