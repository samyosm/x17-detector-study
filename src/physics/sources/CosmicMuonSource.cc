#include "physics/sources/CosmicMuonSource.hh"
#include "G4MuonMinus.hh"
#include "G4MuonPlus.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include <algorithm>
#include <cmath>

CosmicMuonSource::CosmicMuonSource(const Configuration &config)
    : spectrum_(config), plane_(config),
      angularPower_(config.Get<double>("source.cosmic_muons.angular.power")),
      positiveFraction_(
          config.Get<double>("source.cosmic_muons.positive_fraction")) {}

void CosmicMuonSource::Generate(G4Event *event) {
  const double power = angularPower_ + 1.0;
  const double lower = std::pow(spectrum_.MinimumZenithCosine(), power);
  const double cosine =
      std::pow(lower + (1.0 - lower) * G4UniformRand(), 1.0 / power);
  const double sine = std::sqrt(std::max(0.0, 1.0 - cosine * cosine));
  const double azimuth = twopi * G4UniformRand();
  if (G4UniformRand() < positiveFraction_)
    gun_.SetParticleDefinition(G4MuonPlus::Definition());
  else
    gun_.SetParticleDefinition(G4MuonMinus::Definition());

  gun_.SetParticleEnergy(spectrum_.SampleEnergyGeV(cosine) * GeV);
  gun_.SetParticlePosition({(G4UniformRand() - 0.5) * plane_.sideLength,
                            plane_.height,
                            (G4UniformRand() - 0.5) * plane_.sideLength});
  gun_.SetParticleMomentumDirection(
      {sine * std::cos(azimuth), -cosine, sine * std::sin(azimuth)});
  gun_.GeneratePrimaryVertex(event);
}
