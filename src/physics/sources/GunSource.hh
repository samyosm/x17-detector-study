#pragma once

#include "physics/sources/PrimarySource.hh"
#include "configuration/Configuration.hh"

#include <memory>

class G4ParticleGun;

class GunSource final : public PrimarySource {
public:
    explicit GunSource(const Configuration& settings);
    ~GunSource() override;
    void Generate(G4Event* event) override;
    int ModeId() const override { return 0; }

private:
    std::unique_ptr<G4ParticleGun> gun_;
};
