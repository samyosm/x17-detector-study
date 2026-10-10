#include "physics/B4MuonModel.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include <algorithm>
#include <cmath>

namespace {
constexpr double powerIndex = 2.98;
constexpr double scaleEnergyGeV = 3.82;
constexpr double suppressionEnergyGeV = 0.14;
}

B4MuonModel::B4MuonModel(const Configuration &config)
    : minimumCosine_(std::cos(
          config.Get<double>("source.cosmic_muons.angular.zenith_max_deg") *
          degree)),
      minimumEnergyGeV_(config.Get<double>("source.cosmic_muons.energy.min_gev")),
      maximumEnergyGeV_(config.Get<double>("source.cosmic_muons.energy.max_gev")) {}

double B4MuonModel::energyDensity(double energyGeV, double zenithCosine) const {
  return std::pow(scaleEnergyGeV / zenithCosine + energyGeV, -powerIndex) *
         std::exp(-suppressionEnergyGeV / (energyGeV * zenithCosine));
}

MuonSample B4MuonModel::Sample() const {
  const double minimumDensity = minimumCosine_ * minimumCosine_;
  while (true) {
    const double cosine =
        minimumCosine_ + (1 - minimumCosine_) * G4UniformRand();
    if (minimumDensity + (1 - minimumDensity) * G4UniformRand() >=
        cosine * cosine)
      continue;
    const double scale = scaleEnergyGeV / cosine;
    const double suppression = suppressionEnergyGeV / cosine;
    const double peakEnergy =
        suppression +
        std::sqrt(suppression * suppression +
                  4 * scale * suppression * powerIndex) /
            (2 * powerIndex);
    const double bound = energyDensity(
        std::clamp(peakEnergy, minimumEnergyGeV_, maximumEnergyGeV_), cosine);
    const double energy =
        minimumEnergyGeV_ +
        (maximumEnergyGeV_ - minimumEnergyGeV_) * G4UniformRand();
    if (G4UniformRand() * bound < energyDensity(energy, cosine))
      return {cosine, energy};
  }
}
