#pragma once

#include "PrimarySource.hh"
#include "SourceSettings.hh"

#include <memory>

class G4ParticleGun;

class GunSource final : public PrimarySource {
public:
    explicit GunSource(const SourceSettings& settings);
    ~GunSource() override;
    void Generate(G4Event* event) override;
    int ModeId() const override { return 0; }

private:
    std::unique_ptr<G4ParticleGun> gun_;
};
