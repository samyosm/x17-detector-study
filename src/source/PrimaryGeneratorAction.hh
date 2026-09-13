#pragma once

#include "G4VUserPrimaryGeneratorAction.hh"
#include "PrimarySource.hh"
#include "SourceSettings.hh"

#include <memory>

class PrimaryGeneratorAction final : public G4VUserPrimaryGeneratorAction {
public:
  explicit PrimaryGeneratorAction(const SourceSettings &settings);
  ~PrimaryGeneratorAction() override;
  void GeneratePrimaries(G4Event *event) override;
  int GetModeId() const { return source_->ModeId(); }

private:
  std::unique_ptr<PrimarySource> source_;
};
