#pragma once
#include "G4ParticleGun.hh"
#include "physics/LabSourceDefinition.hh"
#include "physics/sources/PrimarySource.hh"

class LabSource final : public PrimarySource {
public:
  explicit LabSource(const Configuration &config);
  void Generate(G4Event *event) override;
  int ModeId() const override { return 4; }
  int LabId() const override { return source_.labId; }

private:
  LabSourceDefinition source_;
  bool ready_ = false;
  G4ParticleGun gun_{1};
};
