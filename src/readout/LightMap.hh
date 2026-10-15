#pragma once
#include "G4ThreeVector.hh"
#include "configuration/Configuration.hh"
#include <array>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

// Half-dimensions of the scintillator bar solid (a trapezoid); local y runs along the bar,
// local z through its thickness, and the width (x) changes linearly with z.
struct BarShape {
  double halfWidthAtBack;  // half x length at local z = -halfDepth
  double halfWidthAtFront; // half x length at local z = +halfDepth
  double halfLength;
  double halfDepth;
  double HalfWidth(double z) const;
};
BarShape FindBarShape();

// Probability that a photon emitted at a point of the bar reaches the D (local +y) or the U
// (local -y) end face, tabulated on a grid in (y, depth, normalized width).
class LightMap {
public:
  LightMap(int bandsY, int bandsDepth, int bandsWidth);

  static const LightMap &Shared(const Configuration &config);
  static std::filesystem::path File(const Configuration &config);
  static std::map<std::string, std::string> Describe(const Configuration &config);

  int BandsY() const { return ny_; }
  int BandsDepth() const { return nz_; }
  int BandsWidth() const { return nx_; }
  std::size_t CellCount() const { return static_cast<std::size_t>(ny_) * nz_ * nx_; }
  std::size_t CellIndex(int iy, int iz, int ix) const { return (static_cast<std::size_t>(iy) * nz_ + iz) * nx_ + ix; }
  // Local point inside the bar at fractional cell coordinates in [0, 1) along each axis.
  static G4ThreeVector LocalPoint(const BarShape &shape, double fy, double fz, double fx);
  std::array<double, 2> Collection(const BarShape &shape, const G4ThreeVector &local) const;

  void Set(std::size_t cell, double d, double u) {
    collection_[0][cell] = static_cast<float>(d);
    collection_[1][cell] = static_cast<float>(u);
  }
  void Save(const std::filesystem::path &file, const std::map<std::string, std::string> &metadata) const;

private:
  int ny_, nz_, nx_;
  std::array<std::vector<float>, 2> collection_;
};
