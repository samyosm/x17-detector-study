#pragma once

#include "PrimarySource.hh"

// Shared two-body recoil and isotropic e+e- decay kinematics for pair modes.
class PairSource : public PrimarySource {
public:
  explicit PairSource(double transitionEnergy);
  void Generate(G4Event *event) final;

protected:
  virtual double SamplePairMass(double parentMass, double daughterMass) = 0;
  static double TwoBodyMomentum(double parentMass, double daughterMass,
                                double pairMass);
  double transitionEnergy_;
};
