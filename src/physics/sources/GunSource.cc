#include "physics/sources/GunSource.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"

#include "G4SystemOfUnits.hh"
#include <stdexcept>

GunSource::GunSource(const Configuration &settings)
    : gun_(std::make_unique<G4ParticleGun>(1)) {
  auto *particle = G4ParticleTable::GetParticleTable()->FindParticle(
      settings.Get<std::string>("source.gun.particle"));
  if (!particle)
    throw std::runtime_error("Unknown gun particle: " +
                             settings.Get<std::string>("source.gun.particle"));
  gun_->SetParticleDefinition(particle);
  gun_->SetParticleEnergy(settings.Get<double>("source.gun.energy_mev") * MeV);
  const auto position = settings.Vector("source.gun.position_cm");
  const auto direction = settings.Vector("source.gun.direction");
  gun_->SetParticlePosition(
      {position[0] * cm, position[1] * cm, position[2] * cm});
  gun_->SetParticleMomentumDirection(
      {direction[0], direction[1], direction[2]});
}

GunSource::~GunSource() = default;

void GunSource::Generate(G4Event *event) { gun_->GeneratePrimaryVertex(event); }
