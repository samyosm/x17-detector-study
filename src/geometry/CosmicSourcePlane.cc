#include "geometry/CosmicSourcePlane.hh"
#include "G4SystemOfUnits.hh"
#include "G4VSolid.hh"
#include <stdexcept>

namespace {
double SquareSideCm(const Configuration &config) {
  return config.Get<double>("source.cosmic_muons.plane.side_cm", 0.0);
}
}

CosmicSourcePlane::CosmicSourcePlane(const Configuration &config)
    : lengthX(config.Get<double>("source.cosmic_muons.plane.x_cm", SquareSideCm(config)) * cm),
      lengthZ(config.Get<double>("source.cosmic_muons.plane.z_cm", SquareSideCm(config)) * cm),
      height(config.Get<double>("source.cosmic_muons.plane.height_cm") * cm) {
  if (!(lengthX > 0 && lengthZ > 0))
    throw std::invalid_argument("Cosmic source plane needs side_cm, or x_cm and z_cm");
}

std::array<G4ThreeVector, 4> CosmicSourcePlane::Corners() const {
  const double halfX = lengthX / 2, halfZ = lengthZ / 2;
  return {{{-halfX, height, -halfZ},
           {halfX, height, -halfZ},
           {halfX, height, halfZ},
           {-halfX, height, halfZ}}};
}

void CosmicSourcePlane::RequireInside(const G4VSolid &world) const {
  for (const auto &corner : Corners())
    if (world.Inside(corner) != kInside)
      throw std::runtime_error("Cosmic source plane must lie inside the world");
}
