#pragma once
#include "G4VUserActionInitialization.hh"
#include "configuration/Configuration.hh"

class ActionInitialization final : public G4VUserActionInitialization {
public:
  explicit ActionInitialization(const Configuration &config)
      : config_(config) {}
  void BuildForMaster() const override;
  void Build() const override;

private:
  Configuration config_;
};
