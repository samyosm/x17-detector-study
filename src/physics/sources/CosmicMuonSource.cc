#include "physics/sources/CosmicMuonSource.hh"
#include "physics/B4MuonModel.hh"
#include "physics/CosmicMuonSpectrum.hh"
#include "G4MuonMinus.hh"
#include "G4MuonPlus.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include <algorithm>
#include <cmath>

CosmicMuonSource::CosmicMuonSource(const Configuration &config)
    : model_(UsesB4Model(config)
                 ? std::unique_ptr<MuonModel>(new B4MuonModel(config))
                 : std::unique_ptr<MuonModel>(new CosmicMuonSpectrum(config))),
      plane_(config),
      positiveFraction_(
          config.Get<double>("source.cosmic_muons.positive_fraction")) {}

void CosmicMuonSource::Generate(G4Event *event) {
  const auto [cosine, energyGeV] = model_->Sample();
  const double sine = std::sqrt(std::max(0.0, 1.0 - cosine * cosine));
  const double azimuth = twopi * G4UniformRand();
  if (G4UniformRand() < positiveFraction_)
    gun_.SetParticleDefinition(G4MuonPlus::Definition());
  else
    gun_.SetParticleDefinition(G4MuonMinus::Definition());

  gun_.SetParticleEnergy(energyGeV * GeV);
  gun_.SetParticlePosition({(G4UniformRand() - 0.5) * plane_.lengthX,
                            plane_.height,
                            (G4UniformRand() - 0.5) * plane_.lengthZ});
  gun_.SetParticleMomentumDirection(
      {sine * std::cos(azimuth), -cosine, sine * std::sin(azimuth)});
  gun_.GeneratePrimaryVertex(event);
}
