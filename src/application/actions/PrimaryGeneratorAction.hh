#pragma once

#include "G4VUserPrimaryGeneratorAction.hh"
#include "physics/sources/PrimarySource.hh"
#include "configuration/Configuration.hh"

#include <memory>

class PrimaryGeneratorAction final : public G4VUserPrimaryGeneratorAction {
public:
  explicit PrimaryGeneratorAction(const Configuration &settings);
  ~PrimaryGeneratorAction() override;
  void GeneratePrimaries(G4Event *event) override;
  int GetModeId() const { return source_->ModeId(); }

private:
  std::unique_ptr<PrimarySource> source_;
};
