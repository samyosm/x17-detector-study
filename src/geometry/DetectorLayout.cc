#include "geometry/DetectorLayout.hh"
#include "G4PhysicalConstants.hh"
#include <cmath>

namespace detector {
Layer FindSensitiveLayer(std::string_view volumeName) {
    if (volumeName == "Scintillator") return scintillator;
    if (volumeName.rfind("DeltaEScintillator_", 0) == 0) return deltaE;
    if (volumeName == "MWPCGas") return chamber;
    return outside;
}

int FindNearestArm(const G4ThreeVector& position) {
    const double azimuth = std::atan2(-position.x(), -position.y());
    const int nearestArm = static_cast<int>(std::floor(azimuth / (twopi / armCount) + 0.5));
    return (nearestArm % armCount + armCount) % armCount;
}

int FindEndSensor(int armIndex, double localLongitudinalPosition) {
    return sensorsPerArm * armIndex + (localLongitudinalPosition > 0 ? 0 : 1);
}
}
