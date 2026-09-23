#pragma once
#include "configuration/Configuration.hh"
#include <vector>

class CosmicMuonSpectrum {
public:
  explicit CosmicMuonSpectrum(const Configuration &config);
  double SampleEnergyGeV(double zenithCosine) const;
  double MinimumZenithCosine() const { return minimumCosine_; }

private:
  struct Spectrum {
    std::vector<double> density, cumulative;
  };
  Spectrum BuildSpectrum(double zenithCosine) const;
  const Spectrum &SelectInterpolatedSpectrum(double zenithCosine) const;
  double InvertCumulativeSpectrum(const Spectrum &spectrum,
                                  double cumulativeArea) const;
  double minimumCosine_;
  std::vector<double> energies_;
  std::vector<Spectrum> spectra_;
};
