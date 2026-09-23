#pragma once
#include "G4ThreeVector.hh"

struct PairDecayProducts {
  G4ThreeVector positron;
  G4ThreeVector electron;
  G4ThreeVector recoil;
  double positronEnergy;
  double electronEnergy;
  double recoilEnergy;
};

PairDecayProducts SamplePairDecay(double pairMass, double pairMomentum,
                                  double daughterMass);
void RequireFourMomentumConservation(const PairDecayProducts &state,
                                     double parentMass);
double CalculateTwoBodyMomentum(double parentMass, double daughterMass,
                                double pairMass);
