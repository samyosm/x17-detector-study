#pragma once

#include "physics/sources/PrimarySource.hh"

class PairSource : public PrimarySource {
public:
  explicit PairSource(double transitionEnergy);
  void Generate(G4Event *event) final;

protected:
  virtual double SamplePairMass(double parentMass, double daughterMass) = 0;
  double transitionEnergy_;
};
