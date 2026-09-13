#pragma once

#include "PairSource.hh"

class X17Source final : public PairSource {
public:
  X17Source(double transitionEnergy, double mass);
  int ModeId() const override { return 2; }

private:
  double SamplePairMass(double parentMass, double daughterMass) override;
  double mass_;
};
