#pragma once
#include "configuration/Configuration.hh"
#include "geometry/DetectorLayout.hh"
#include <array>
#include <vector>

class G4Step;

struct BarHit {
  int bar;
  double energy;
  double position;
  double time;
  double start;
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
  };
  double window_;
  std::array<std::vector<Deposit>, detector::armCount> deposits_;
};
