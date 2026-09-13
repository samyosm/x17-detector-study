#include "IpcSource.hh"

#include "G4PhysicalConstants.hh"
#include "Randomize.hh"

#include <cmath>

double IpcSource::SamplePairMass(double parentMass, double daughterMass) {
  // Log-uniform proposals supply the 1/m factor of this simplified IPC model.
  const double minimumMass = 2.0 * electron_mass_c2;
  const double p0 = TwoBodyMomentum(parentMass, daughterMass, 0.0);
  while (true) {
    const double mass = minimumMass * std::pow(transitionEnergy_ / minimumMass,
                                               G4UniformRand());
    const double ratio = minimumMass * minimumMass / (mass * mass);
    // Lepton phase space times p^3 recoil weighting; no nuclear amplitudes.
    const double leptonFactor = std::sqrt(1.0 - ratio) * (1.0 + ratio / 2.0);
    const double p = TwoBodyMomentum(parentMass, daughterMass, mass);
    if (G4UniformRand() < leptonFactor * std::pow(p / p0, 3))
      return mass;
  }
}
