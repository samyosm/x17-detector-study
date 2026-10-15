#pragma once
#include "configuration/Configuration.hh"
#include "geometry/DetectorLayout.hh"
#include "readout/LightMap.hh"
#include <array>
#include <vector>

class G4EmSaturation;
class G4Step;

struct BarHit {
  int bar;
  double energy;
  double position;
  double time;
  double start;
  std::array<double, 2> photons{}; // expected detected photons at the D and U ends (light-map mode)
};

class BarHitReadout {
public:
  explicit BarHitReadout(const Configuration &config);
  void Reset();
  void RecordDeposit(const G4Step &step, int armIndex, double time);
  std::vector<BarHit> Hits();

private:
  struct Deposit {
    double time;
    double energy;
    double position;
    std::array<double, 2> photons;
  };
  double window_;
  const LightMap *lightMap_ = nullptr;
  G4EmSaturation *saturation_ = nullptr;
  BarShape shape_{};
  double detectedPhotonsPerMeV_ = 0.0;
  std::array<std::vector<Deposit>, detector::armCount> deposits_;
};
