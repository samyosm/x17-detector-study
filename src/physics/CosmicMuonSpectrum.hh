#pragma once
#include "configuration/Configuration.hh"
#include <vector>

class CosmicMuonSpectrum {
public:
  explicit CosmicMuonSpectrum(const Configuration &config);
  double SampleEnergyGeV(double zenithCosine) const;
  double SampleZenithCosine() const;
  double HorizontalFluxPerCm2Second() const;

private:
  struct Spectrum {
    std::vector<double> density, cumulative;
  };
  Spectrum BuildSpectrum(double zenithCosine) const;
  const Spectrum &SelectInterpolatedSpectrum(double zenithCosine) const;
  double InvertCumulativeSpectrum(const Spectrum &spectrum,
                                  double cumulativeArea,
                                  const std::vector<double> &nodes) const;
  double minimumCosine_;
  std::vector<double> energies_, cosines_;
  Spectrum angularSpectrum_;
  std::vector<Spectrum> spectra_;
};
