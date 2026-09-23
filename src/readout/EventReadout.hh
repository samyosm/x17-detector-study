#pragma once
#include "geometry/DetectorLayout.hh"
#include <array>
#include <limits>

struct EventReadout {
  int eventId = 0;
  std::array<double, detector::armCount> depositedEnergy{};
  std::array<int, detector::sensorCount> photonCount{};
  std::array<double, detector::sensorCount> firstPhotonTimeNs;

  explicit EventReadout(int id = 0) : eventId(id) {
    firstPhotonTimeNs.fill(std::numeric_limits<double>::quiet_NaN());
  }
};
