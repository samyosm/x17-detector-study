#include "physics/sources/IpcSource.hh"
#include "physics/PairDecay.hh"

#include "G4PhysicalConstants.hh"
#include "Randomize.hh"

#include <cmath>

double IpcSource::SamplePairMass(double parentMass, double daughterMass) {
  const double minimumMass = 2.0 * electron_mass_c2;
  const double maximumRecoilMomentum =
      CalculateTwoBodyMomentum(parentMass, daughterMass, 0.0);
  while (true) {
    const double mass = minimumMass * std::pow(transitionEnergy_ / minimumMass,
                                               G4UniformRand());
    const double ratio = minimumMass * minimumMass / (mass * mass);

    const double leptonFactor = std::sqrt(1.0 - ratio) * (1.0 + ratio / 2.0);
    const double recoilMomentum =
        CalculateTwoBodyMomentum(parentMass, daughterMass, mass);
    if (G4UniformRand() <
        leptonFactor * std::pow(recoilMomentum / maximumRecoilMomentum, 3))
      return mass;
  }
}
