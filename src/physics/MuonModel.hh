#pragma once
#include "configuration/Configuration.hh"
#include <stdexcept>
#include <string>

struct MuonSample {
  double zenithCosine;
  double energyGeV;
};

class MuonModel {
public:
  virtual ~MuonModel() = default;
  virtual MuonSample Sample() const = 0;
};

inline bool UsesB4Model(const Configuration &config) {
  const auto name = config.Get<std::string>("source.cosmic_muons.model", "guan");
  if (name != "guan" && name != "b4")
    throw std::invalid_argument("source.cosmic_muons.model must be guan or b4");
  return name == "b4";
}
