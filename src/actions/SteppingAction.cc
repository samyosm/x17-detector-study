#include "SteppingAction.hh"
#include "EventAction.hh"

#include "G4LogicalVolume.hh"
#include "G4OpticalPhoton.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4Tubs.hh"
#include "G4VPhysicalVolume.hh"
#include "Randomize.hh"

#include <cmath>

namespace {
bool AtCoupledFace(const G4StepPoint* point) {
  const auto& touchable = point->GetTouchableHandle();
  const auto* volume = touchable->GetVolume();
  const auto* tube = dynamic_cast<const G4Tubs*>(volume->GetLogicalVolume()->GetSolid());
  if (!tube) return false;
  const int channel = touchable->GetCopyNumber();
  // D (odd) touches the bar at its +z face; U (even) at its -z face.
  const double faceZ = touchable->GetTranslation().z() +
                       (channel % 2 ? tube->GetZHalfLength() : -tube->GetZHalfLength());
  return std::abs(point->GetPosition().z() - faceZ) < 0.001 * mm;
}
}

SteppingAction::SteppingAction(EventAction *eventAction, double quantumEfficiency)
    : eventAction_(eventAction), quantumEfficiency_(quantumEfficiency) {}

void SteppingAction::UserSteppingAction(const G4Step *step) {
  auto* track = step->GetTrack();
  const auto &touchable = step->GetPreStepPoint()->GetTouchableHandle();
  const auto *volume = touchable->GetVolume();
  if (track->GetParticleDefinition() == G4OpticalPhoton::Definition()) {
    // The first step inside a PMT proves transmission across the optical boundary.
    if (volume && volume->GetLogicalVolume()->GetName() == "PMT" &&
        step->GetPreStepPoint()->GetStepStatus() == fGeomBoundary) {
      if (AtCoupledFace(step->GetPreStepPoint()) &&
          G4UniformRand() < quantumEfficiency_) {
        eventAction_->AddPmtPhoton(touchable->GetCopyNumber(),
                                   step->GetPreStepPoint()->GetGlobalTime(),
                                   track->GetKineticEnergy());
      }
      track->SetTrackStatus(fStopAndKill);
    }
    return;
  }
  if (volume && volume->GetLogicalVolume()->GetName() == "Scintillator") {
    eventAction_->AddBarEnergy(touchable->GetCopyNumber(),
                               step->GetTotalEnergyDeposit());
  }
}
