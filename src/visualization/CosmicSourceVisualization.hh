#pragma once
#include "G4VUserVisAction.hh"
#include "G4VisExtent.hh"
#include "geometry/CosmicSourcePlane.hh"

class CosmicSourceVisualization final : public G4VUserVisAction {
public:
  explicit CosmicSourceVisualization(const Configuration &config)
      : plane_(config) {}
  G4VisExtent Extent() const;

protected:
  void Draw() override;

private:
  CosmicSourcePlane plane_;
};
