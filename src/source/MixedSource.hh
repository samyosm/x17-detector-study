#pragma once

#include "IpcSource.hh"
#include "X17Source.hh"

class MixedSource final : public PrimarySource {
public:
  MixedSource(double transitionEnergy, double x17Mass, double x17Fraction);
  void Generate(G4Event *event) override;
  int ModeId() const override;

private:
  double x17Fraction_;
  IpcSource ipc_;
  X17Source x17_;
  PrimarySource *selected_;
};
