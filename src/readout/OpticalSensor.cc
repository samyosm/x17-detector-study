#include "readout/OpticalSensor.hh"
#include "G4LogicalVolume.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4TouchableHistory.hh"
#include "G4Trd.hh"
#include "geometry/DetectorLayout.hh"
#include <cmath>

int FindCrossedEndSensor(const G4Step &step) {
  const auto *end = step.GetPostStepPoint();
  if (end->GetStepStatus() != fGeomBoundary)
    return -1;
  const auto &touchable = step.GetPreStepPoint()->GetTouchableHandle();
  const auto *volume = touchable->GetVolume();
  if (!volume ||
      detector::FindSensitiveLayer(volume->GetLogicalVolume()->GetName()) !=
          detector::scintillator)
    return -1;
  const auto *trapezoid =
      dynamic_cast<const G4Trd *>(volume->GetLogicalVolume()->GetSolid());
  if (!trapezoid)
    return -1;
  const auto localPosition =
      touchable->GetHistory()->GetTopTransform().TransformPoint(
          end->GetPosition());
  const double distanceFromEnd =
      std::abs(std::abs(localPosition.y()) - trapezoid->GetYHalfLength1());
  if (distanceFromEnd > 1e-6 * mm)
    return -1;
  return detector::FindEndSensor(touchable->GetCopyNumber(), localPosition.y());
}
