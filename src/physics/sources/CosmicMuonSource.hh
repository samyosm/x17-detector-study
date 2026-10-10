#pragma once
#include "G4ParticleGun.hh"
#include "geometry/CosmicSourcePlane.hh"
#include "physics/MuonModel.hh"
#include "physics/sources/PrimarySource.hh"
#include <memory>

class CosmicMuonSource final : public PrimarySource {
public:
  explicit CosmicMuonSource(const Configuration &config);
  void Generate(G4Event *event) override;
  int ModeId() const override { return 3; }

private:
  std::unique_ptr<MuonModel> model_;
  CosmicSourcePlane plane_;
  double positiveFraction_;
  G4ParticleGun gun_{1};
};
