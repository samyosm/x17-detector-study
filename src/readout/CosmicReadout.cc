#include "readout/CosmicReadout.hh"
#include "G4LogicalVolume.hh"
#include "G4OpticalPhoton.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"
#include "geometry/DetectorLayout.hh"
#include <stdexcept>

CosmicReadout::CosmicReadout(const Configuration &config)
    : thresholds_{config.Get<double>("readout.thresholds.scintillator_mev") *
                      MeV,
                  config.Get<double>("readout.thresholds.delta_e_mev") * MeV,
                  config.Get<double>("readout.thresholds.mwpc_mev") * MeV} {}

void CosmicReadout::Reset() { deposits_ = {}; }

std::optional<detector::SensitiveDetectorAddress>
CosmicReadout::RecordDeposit(const G4Step &step) {
  if (step.GetTrack()->GetDefinition() == G4OpticalPhoton::Definition())
    return std::nullopt;
  const double energy = step.GetTotalEnergyDeposit();
  if (energy <= 0)
    return std::nullopt;
  const auto &touchable = step.GetPreStepPoint()->GetTouchableHandle();
  if (!touchable->GetVolume())
    return std::nullopt;
  const auto layer = detector::FindSensitiveLayer(
      touchable->GetVolume()->GetLogicalVolume()->GetName());
  if (layer == detector::outside)
    return std::nullopt;
  const auto position = (step.GetPreStepPoint()->GetPosition() +
                         step.GetPostStepPoint()->GetPosition()) /
                        2;
  const int arm = layer == detector::chamber
                      ? detector::FindNearestArm(position)
                      : touchable->GetCopyNumber();
  if (arm < 0 || arm >= detector::armCount)
    throw std::runtime_error("Invalid detector arm copy");
  deposits_[layer][arm].Add(energy, position);
  return detector::SensitiveDetectorAddress{layer, arm};
}
