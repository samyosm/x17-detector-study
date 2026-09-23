#pragma once
#include <array>
class G4Event;

struct PairObservables {
  double massMeV;
  double openingAngleDegrees;
  double positronEnergyMeV;
  double electronEnergyMeV;
  std::array<double, 3> positronMomentumMeV;
  std::array<double, 3> electronMomentumMeV;
};
PairObservables CalculatePrimaryPairObservables(const G4Event &event);
