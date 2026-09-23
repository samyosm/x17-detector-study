#pragma once
#include "readout/DetectorDeposits.hh"
#include <vector>

struct CosmicObservables {
  std::vector<int> hitArms;
  int hitMask = 0;
  double energySum;
  double energyAsymmetry;
  double openingAngle;
  double offsetOpeningAngle;
};
CosmicObservables
CalculateCosmicObservables(const DetectorDeposits &deposits,
                           const EnergyThresholds &thresholds);
