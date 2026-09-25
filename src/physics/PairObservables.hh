#pragma once
#include <array>
#include <limits>
class G4Event;

struct PairObservables {
  static constexpr double missing = std::numeric_limits<double>::quiet_NaN();
  double massMeV = missing;
  double openingAngleDegrees = missing;
  double positronEnergyMeV = missing;
  double electronEnergyMeV = missing;
  std::array<double, 3> positronMomentumMeV{missing, missing, missing};
  std::array<double, 3> electronMomentumMeV{missing, missing, missing};
};
PairObservables CalculatePrimaryPairObservables(const G4Event &event);
