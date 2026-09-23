#pragma once

#include "physics/sources/PairSource.hh"

class IpcSource final : public PairSource {
public:
  using PairSource::PairSource;
  int ModeId() const override { return 1; }

private:
  double SamplePairMass(double parentMass, double daughterMass) override;
};
