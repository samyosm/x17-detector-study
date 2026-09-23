#include "physics/CosmicObservables.hh"
#include "G4SystemOfUnits.hh"
#include <cmath>
#include <limits>

namespace {
bool PassesAllThresholds(const DetectorDeposits &deposits,
                         const EnergyThresholds &thresholds, int arm) {
  for (int layer = 0; layer < detector::layerCount; ++layer)
    if (!(deposits[layer][arm].energy > thresholds[layer]))
      return false;
  return true;
}

double ScintillatorEnergy(const DetectorDeposits &deposits, int arm) {
  return deposits[detector::scintillator][arm].energy +
         deposits[detector::deltaE][arm].energy;
}
} // namespace

CosmicObservables
CalculateCosmicObservables(const DetectorDeposits &deposits,
                           const EnergyThresholds &thresholds) {
  const double missing = std::numeric_limits<double>::quiet_NaN();
  CosmicObservables result{{}, 0, missing, missing, missing, missing};
  for (int arm = 0; arm < detector::armCount; ++arm) {
    if (!PassesAllThresholds(deposits, thresholds, arm))
      continue;
    result.hitArms.push_back(arm);
    result.hitMask |= 1 << arm;
  }
  if (result.hitArms.size() != 2)
    return result;
  const int firstArm = result.hitArms[0];
  const int secondArm = result.hitArms[1];
  const double firstEnergy = ScintillatorEnergy(deposits, firstArm);
  const double secondEnergy = ScintillatorEnergy(deposits, secondArm);
  result.energySum = firstEnergy + secondEnergy;
  result.energyAsymmetry =
      std::abs(firstEnergy - secondEnergy) / result.energySum;
  const auto firstPosition = deposits[detector::chamber][firstArm].Centroid();
  const auto secondPosition = deposits[detector::chamber][secondArm].Centroid();
  result.openingAngle = firstPosition.angle(secondPosition);
  const G4ThreeVector heliumVertexOffset(0, 0, 25 * mm);
  result.offsetOpeningAngle = (firstPosition - heliumVertexOffset)
                                  .angle(secondPosition - heliumVertexOffset);
  return result;
}
