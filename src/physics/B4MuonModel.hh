#pragma once
#include "configuration/Configuration.hh"
#include "physics/MuonModel.hh"

class B4MuonModel final : public MuonModel {
public:
  explicit B4MuonModel(const Configuration &config);
  MuonSample Sample() const override;

private:
  double energyDensity(double energyGeV, double zenithCosine) const;
  double minimumCosine_, minimumEnergyGeV_, maximumEnergyGeV_;
};
