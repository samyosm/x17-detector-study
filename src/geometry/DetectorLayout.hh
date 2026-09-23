#pragma once
#include "G4ThreeVector.hh"
#include <string_view>

namespace detector {
inline constexpr int armCount = 16;
inline constexpr int sensorsPerArm = 2;
inline constexpr int sensorCount = armCount * sensorsPerArm;
inline constexpr int layerCount = 3;
enum Layer { scintillator = 0, deltaE = 1, chamber = 2, outside = -1 };
struct SensitiveDetectorAddress {
  Layer layer;
  int armIndex;
};
Layer FindSensitiveLayer(std::string_view volumeName);
int FindNearestArm(const G4ThreeVector &position);
int FindEndSensor(int armIndex, double localLongitudinalPosition);
} // namespace detector
