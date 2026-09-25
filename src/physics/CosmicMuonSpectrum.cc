#include "physics/CosmicMuonSpectrum.hh"
#include "G4MuonPlus.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
double EffectiveZenithCosine(double cosine) {
  return std::sqrt((cosine * cosine + 0.102573 * 0.102573 -
                 0.068287 * std::pow(cosine, 0.958633) +
                 0.0407253 * std::pow(cosine, 0.817285)) /
                (1 + 0.102573 * 0.102573 - 0.068287 + 0.0407253));
}

double EvaluateGuanFlux(double totalEnergyGeV, double effectiveCosine,
                       double energyShiftGeV) {
  const double correctedEnergyGeV = totalEnergyGeV + energyShiftGeV;
  return 0.14 * std::pow(correctedEnergyGeV, -2.7) *
         (1 / (1 + 1.1 * totalEnergyGeV * effectiveCosine / 115) +
          0.054 / (1 + 1.1 * totalEnergyGeV * effectiveCosine / 850));
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

  if (energyNodes < 2 || cosineNodes < 2 ||
      !(minEnergy > 0 && maxEnergy > minEnergy && std::isfinite(maxEnergy)) ||
      !(zenith > 0 && zenith <= 90))
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
    cosines_.push_back(cosine);
    spectra_.push_back(BuildSpectrum(cosine));
    angularSpectrum_.density.push_back(spectra_.back().cumulative.back());
    const double area = j == 0 ? 0
                               : (cosines_[j] - cosines_[j - 1]) *
                                     (angularSpectrum_.density[j] +
                                      angularSpectrum_.density[j - 1]) /
                                     2;
    angularSpectrum_.cumulative.push_back(
        j == 0 ? 0 : angularSpectrum_.cumulative.back() + area);
  }
}

CosmicMuonSpectrum::Spectrum
CosmicMuonSpectrum::BuildSpectrum(double zenithCosine) const {
  const double effectiveCosine = EffectiveZenithCosine(zenithCosine);
  const double energyShiftGeV = 3.64 / std::pow(effectiveCosine, 1.29);
  const double massGeV = G4MuonPlus::Definition()->GetPDGMass() / GeV;
  Spectrum spectrum;
  spectrum.cumulative.push_back(0.0);
  for (std::size_t energyIndex = 0; energyIndex < energies_.size();
       ++energyIndex) {
    spectrum.density.push_back(
        zenithCosine * EvaluateGuanFlux(energies_[energyIndex] + massGeV,
                                      effectiveCosine, energyShiftGeV));
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

double CosmicMuonSpectrum::InvertCumulativeSpectrum(
    const Spectrum &spectrum, double cumulativeArea,
    const std::vector<double> &nodes) const {
  const auto upperBound = std::upper_bound(
      spectrum.cumulative.begin(), spectrum.cumulative.end(), cumulativeArea);
  const auto interval = std::min(
      static_cast<std::size_t>(upperBound - spectrum.cumulative.begin() - 1),
      nodes.size() - 2);
  const double width = nodes[interval + 1] - nodes[interval];
  const double startDensity = spectrum.density[interval];
  const double slope = (spectrum.density[interval + 1] - startDensity) / width;
  const double remainingArea = cumulativeArea - spectrum.cumulative[interval];
  if (remainingArea <= 0)
    return nodes[interval];
  const double distance =
      2 * remainingArea /
      (startDensity + std::sqrt(std::max(0.0, startDensity * startDensity +
                                                  2 * slope * remainingArea)));
  return nodes[interval] + std::clamp(distance, 0.0, width);
}

double CosmicMuonSpectrum::SampleEnergyGeV(double zenithCosine) const {
  const auto &spectrum = SelectInterpolatedSpectrum(zenithCosine);
  return InvertCumulativeSpectrum(
      spectrum, G4UniformRand() * spectrum.cumulative.back(), energies_);
}

double CosmicMuonSpectrum::SampleZenithCosine() const {
  return InvertCumulativeSpectrum(
      angularSpectrum_, G4UniformRand() * angularSpectrum_.cumulative.back(),
      cosines_);
}

double CosmicMuonSpectrum::HorizontalFluxPerCm2Second() const {
  return twopi * angularSpectrum_.cumulative.back();
}
