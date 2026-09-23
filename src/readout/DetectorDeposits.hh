#pragma once
#include "G4ThreeVector.hh"
#include "geometry/DetectorLayout.hh"
#include <array>
#include <limits>

struct EnergyDeposit {
  double energy = 0;
  G4ThreeVector energyWeightedPosition;

  void Add(double depositedEnergy, const G4ThreeVector &position) {
    energy += depositedEnergy;
    energyWeightedPosition += depositedEnergy * position;
  }

  G4ThreeVector Centroid() const {
    if (energy > 0)
      return energyWeightedPosition / energy;
    const double missing = std::numeric_limits<double>::quiet_NaN();
    return {missing, missing, missing};
  }
};

using DetectorDeposits =
    std::array<std::array<EnergyDeposit, detector::armCount>,
               detector::layerCount>;
using EnergyThresholds = std::array<double, detector::layerCount>;
