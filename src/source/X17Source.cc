#include "X17Source.hh"

#include "G4PhysicalConstants.hh"

#include <stdexcept>

X17Source::X17Source(double transitionEnergy, double mass)
    : PairSource(transitionEnergy), mass_(mass) {
  if (mass_ <= 2.0 * electron_mass_c2 || mass_ >= transitionEnergy_) {
    throw std::invalid_argument(
        "X17 mass must lie between two electron masses and transition energy");
  }
}

double X17Source::SamplePairMass(double, double) { return mass_; }
