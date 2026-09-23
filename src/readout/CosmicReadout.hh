#pragma once
#include "configuration/Configuration.hh"
#include "readout/DetectorDeposits.hh"
class G4Step;
#include <optional>

class CosmicReadout {
public:
  explicit CosmicReadout(const Configuration &config);
  void Reset();
  std::optional<detector::SensitiveDetectorAddress>
  RecordDeposit(const G4Step &step);
  const DetectorDeposits &Deposits() const { return deposits_; }
  const EnergyThresholds &Thresholds() const { return thresholds_; }

private:
  EnergyThresholds thresholds_;
  DetectorDeposits deposits_{};
};
