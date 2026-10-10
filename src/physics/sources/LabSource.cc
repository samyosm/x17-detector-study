#include "physics/sources/LabSource.hh"
#include "G4IonTable.hh"
#include "Randomize.hh"

LabSource::LabSource(const Configuration &config) : source_(ReadLabSource(config)) {
  gun_.SetParticleEnergy(0);
  gun_.SetParticleMomentumDirection({0, 0, 1});
}

void LabSource::Generate(G4Event *event) {
  if (!ready_) {
    gun_.SetParticleDefinition(G4IonTable::GetIonTable()->GetIon(
        source_.atomicNumber, source_.massNumber, 0.0));
    ready_ = true;
  }
  const G4ThreeVector position(
      source_.placementMinimum.x() + G4UniformRand() * (source_.placementMaximum.x() - source_.placementMinimum.x()),
      source_.placementMinimum.y() + G4UniformRand() * (source_.placementMaximum.y() - source_.placementMinimum.y()),
      source_.placementMinimum.z() + G4UniformRand() * (source_.placementMaximum.z() - source_.placementMinimum.z()));
  gun_.SetParticlePosition(position);
  gun_.GeneratePrimaryVertex(event);
}
