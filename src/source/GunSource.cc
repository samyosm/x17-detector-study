#include "GunSource.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"

#include <stdexcept>

GunSource::GunSource(const SourceSettings& settings)
    : gun_(std::make_unique<G4ParticleGun>(1)) {
    auto* particle = G4ParticleTable::GetParticleTable()->FindParticle(settings.gunParticle);
    if (!particle) throw std::runtime_error("Unknown gun particle: " + settings.gunParticle);
    gun_->SetParticleDefinition(particle);
    gun_->SetParticleEnergy(settings.gunEnergy);
    gun_->SetParticlePosition(settings.gunPosition);
    gun_->SetParticleMomentumDirection(settings.gunDirection);
}

GunSource::~GunSource() = default;

void GunSource::Generate(G4Event* event) {
    // Geant4 creates the configured single-particle primary vertex.
    gun_->GeneratePrimaryVertex(event);
}
