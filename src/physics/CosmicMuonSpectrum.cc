#include "physics/CosmicMuonSpectrum.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
double EvaluateGaisserFlux(double energyGeV, double cosine) {
  return 0.14 * std::pow(energyGeV, -2.7) *
         (1.0 / (1.0 + 1.1 * energyGeV * cosine / 115.0) +
          0.054 / (1.0 + 1.1 * energyGeV * cosine / 850.0));
}
} // namespace

CosmicMuonSpectrum::CosmicMuonSpectrum(const Configuration &config)
    : minimumCosine_(std::cos(
          config.Get<double>("source.cosmic_muons.angular.zenith_max_deg") *
          degree)) {
  const auto energyNodes = config.Get<int>("source.cosmic_muons.energy.nodes");
  const auto cosineNodes = config.Get<int>("source.cosmic_muons.angular.nodes");
  const auto minEnergy =
      config.Get<double>("source.cosmic_muons.energy.min_gev");
  const auto maxEnergy =
      config.Get<double>("source.cosmic_muons.energy.max_gev");
  const auto zenith =
      config.Get<double>("source.cosmic_muons.angular.zenith_max_deg");
  const auto power = config.Get<double>("source.cosmic_muons.angular.power");

  if (energyNodes < 2 || cosineNodes < 2 ||
      !(minEnergy > 0 && maxEnergy > minEnergy && std::isfinite(maxEnergy)) ||
      !(zenith > 0 && zenith < 90) || !(power > -1 && std::isfinite(power)))
    throw std::invalid_argument(
        "Invalid cosmic energy grid or angular distribution");
  for (int i = 0; i < energyNodes; ++i) {
    energies_.push_back(minEnergy *
                        std::pow(maxEnergy / minEnergy,
                                 static_cast<double>(i) / (energyNodes - 1)));
  }
  for (int j = 0; j < cosineNodes; ++j) {
    const double cosine =
        minimumCosine_ + (1.0 - minimumCosine_) * j / (cosineNodes - 1);
    spectra_.push_back(BuildSpectrum(cosine));
  }
}

CosmicMuonSpectrum::Spectrum
CosmicMuonSpectrum::BuildSpectrum(double zenithCosine) const {
  Spectrum spectrum;
  spectrum.cumulative.push_back(0.0);
  for (std::size_t energyIndex = 0; energyIndex < energies_.size();
       ++energyIndex) {
    spectrum.density.push_back(
        EvaluateGaisserFlux(energies_[energyIndex], zenithCosine));
    if (energyIndex == 0)
      continue;
    const double intervalWidth =
        energies_[energyIndex] - energies_[energyIndex - 1];
    const double endpointDensitySum =
        spectrum.density[energyIndex] + spectrum.density[energyIndex - 1];
    spectrum.cumulative.push_back(spectrum.cumulative.back() +
                                  intervalWidth * endpointDensitySum / 2.0);
  }
  return spectrum;
}

const CosmicMuonSpectrum::Spectrum &
CosmicMuonSpectrum::SelectInterpolatedSpectrum(double zenithCosine) const {
  const double gridPosition =
      std::clamp((zenithCosine - minimumCosine_) / (1.0 - minimumCosine_) *
                     (spectra_.size() - 1),
                 0.0, static_cast<double>(spectra_.size() - 1));
  const auto lowerIndex =
      std::min(static_cast<std::size_t>(gridPosition), spectra_.size() - 2);
  const double upperFraction = gridPosition - lowerIndex;
  const double lowerWeight =
      (1.0 - upperFraction) * spectra_[lowerIndex].cumulative.back();
  const double upperWeight =
      upperFraction * spectra_[lowerIndex + 1].cumulative.back();
  const bool chooseUpper =
      G4UniformRand() * (lowerWeight + upperWeight) >= lowerWeight;
  return spectra_[lowerIndex + (chooseUpper ? 1 : 0)];
}

double
CosmicMuonSpectrum::InvertCumulativeSpectrum(const Spectrum &spectrum,
                                             double cumulativeArea) const {
  const auto upperBound = std::upper_bound(
      spectrum.cumulative.begin(), spectrum.cumulative.end(), cumulativeArea);
  const auto interval = std::min(
      static_cast<std::size_t>(upperBound - spectrum.cumulative.begin() - 1),
      energies_.size() - 2);
  const double width = energies_[interval + 1] - energies_[interval];
  const double startDensity = spectrum.density[interval];
  const double slope = (spectrum.density[interval + 1] - startDensity) / width;
  const double remainingArea = cumulativeArea - spectrum.cumulative[interval];
  const double distance =
      2 * remainingArea /
      (startDensity + std::sqrt(std::max(0.0, startDensity * startDensity +
                                                  2 * slope * remainingArea)));
  return energies_[interval] + std::clamp(distance, 0.0, width);
}

double CosmicMuonSpectrum::SampleEnergyGeV(double zenithCosine) const {
  const auto &spectrum = SelectInterpolatedSpectrum(zenithCosine);
  return InvertCumulativeSpectrum(spectrum,
                                  G4UniformRand() * spectrum.cumulative.back());
}
