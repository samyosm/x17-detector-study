#include "physics/PairDecay.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
G4ThreeVector SampleIsotropicDirection() {
  const double cosine = 2.0 * G4UniformRand() - 1.0;
  const double sine = std::sqrt(1.0 - cosine * cosine);
  const double phi = twopi * G4UniformRand();
  return {sine * std::cos(phi), sine * std::sin(phi), cosine};
}

G4ThreeVector BoostMomentum(const G4ThreeVector &restMomentum,
                            double restEnergy, const G4ThreeVector &direction,
                            double beta, double gamma) {
  const double parallel = restMomentum.dot(direction);
  return restMomentum +
         ((gamma - 1.0) * parallel + gamma * beta * restEnergy) * direction;
}

} // namespace

PairDecayProducts SamplePairDecay(double pairMass, double pairMomentum,
                                  double daughterMass) {
  const double pairEnergy = std::hypot(pairMass, pairMomentum);
  const double beta = pairMomentum / pairEnergy;
  const double gamma = pairEnergy / pairMass;

  const double leptonEnergy = pairMass / 2.0;
  const double leptonMomentum = std::sqrt(leptonEnergy * leptonEnergy -
                                          electron_mass_c2 * electron_mass_c2);
  const auto direction = SampleIsotropicDirection();
  const auto positronRest = leptonMomentum * SampleIsotropicDirection();
  return {BoostMomentum(positronRest, leptonEnergy, direction, beta, gamma),
          BoostMomentum(-positronRest, leptonEnergy, direction, beta, gamma),
          -pairMomentum * direction,
          gamma * (leptonEnergy + beta * positronRest.dot(direction)),
          gamma * (leptonEnergy - beta * positronRest.dot(direction)),
          std::hypot(daughterMass, pairMomentum)};
}

void RequireFourMomentumConservation(const PairDecayProducts &state,
                                     double parentMass) {
  if (std::abs(state.positronEnergy + state.electronEnergy +
               state.recoilEnergy - parentMass) > 1e-6 * MeV ||
      (state.positron + state.electron + state.recoil).mag() > 1e-6 * MeV) {
    throw std::runtime_error("Generated pair does not conserve four-momentum");
  }
}

double CalculateTwoBodyMomentum(double parentMass, double daughterMass,
                                double pairMass) {
  const double first = parentMass * parentMass -
                       (daughterMass + pairMass) * (daughterMass + pairMass);
  const double second = parentMass * parentMass -
                        (daughterMass - pairMass) * (daughterMass - pairMass);
  return std::sqrt(std::max(0.0, first * second)) / (2.0 * parentMass);
}
