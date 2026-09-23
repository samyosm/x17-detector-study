#include "geometry/CosmicSourcePlane.hh"
#include "G4SystemOfUnits.hh"
#include "G4VSolid.hh"
#include <stdexcept>

CosmicSourcePlane::CosmicSourcePlane(const Configuration &config)
    : sideLength(config.Get<double>("source.cosmic_muons.plane.side_cm") * cm),
      height(config.Get<double>("source.cosmic_muons.plane.height_cm") * cm) {}

std::array<G4ThreeVector, 4> CosmicSourcePlane::Corners() const {
  const double halfSide = sideLength / 2;
  return {{{-halfSide, height, -halfSide},
           {halfSide, height, -halfSide},
           {halfSide, height, halfSide},
           {-halfSide, height, halfSide}}};
}

void CosmicSourcePlane::RequireInside(const G4VSolid &world) const {
  for (const auto &corner : Corners())
    if (world.Inside(corner) != kInside)
      throw std::runtime_error("Cosmic source plane must lie inside the world");
}
