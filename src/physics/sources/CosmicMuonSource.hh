#pragma once
#include "G4ParticleGun.hh"
#include "geometry/CosmicSourcePlane.hh"
#include "physics/CosmicMuonSpectrum.hh"
#include "physics/sources/PrimarySource.hh"

class CosmicMuonSource final : public PrimarySource {
public:
  explicit CosmicMuonSource(const Configuration &config);
  void Generate(G4Event *event) override;
  int ModeId() const override { return 3; }

private:
  CosmicMuonSpectrum spectrum_;
  CosmicSourcePlane plane_;
  double positiveFraction_;
  G4ParticleGun gun_{1};
};
