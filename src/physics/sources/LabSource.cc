#include "physics/sources/LabSource.hh"
#include "G4IonTable.hh"
#include "Randomize.hh"
#include "geometry/SourcePose.hh"
#include <cmath>

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
  const auto &placement = source_.placement;
  G4ThreeVector position;
  if (source_.encapsulated) {
    const auto &pose = SourcePose::Instance();
    const double radius = source_.capsule.activeRadius * std::sqrt(G4UniformRand());
    const double phi = CLHEP::twopi * G4UniformRand();
    const G4ThreeVector local(radius * std::cos(phi), radius * std::sin(phi),
                              (G4UniformRand() - 0.5) * source_.capsule.activeThickness);
    position = pose.Position() + pose.Rotation() * local;
  } else {
    const double z = placement.uniformZ
                         ? placement.zMinimum + G4UniformRand() * (placement.zMaximum - placement.zMinimum)
                         : placement.zFixed;
    position = {placement.x, placement.y, z};
  }
  gun_.SetParticlePosition(position);
  gun_.GeneratePrimaryVertex(event);
}
