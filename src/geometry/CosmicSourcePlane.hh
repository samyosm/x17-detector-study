#pragma once
#include "G4ThreeVector.hh"
#include "configuration/Configuration.hh"
#include <array>

class G4VSolid;

struct CosmicSourcePlane {
  double sideLength;
  double height;
  explicit CosmicSourcePlane(const Configuration &config);
  std::array<G4ThreeVector, 4> Corners() const;
  void RequireInside(const G4VSolid &world) const;
};
