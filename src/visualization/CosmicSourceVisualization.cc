#include "visualization/CosmicSourceVisualization.hh"
#include "G4Polyline.hh"
#include "G4VVisManager.hh"
#include "G4VisAttributes.hh"

G4VisExtent CosmicSourceVisualization::Extent() const {
  const auto halfSide = plane_.sideLength / 2;
  return {-halfSide,     halfSide,  plane_.height,
          plane_.height, -halfSide, halfSide};
}

void CosmicSourceVisualization::Draw() {
  auto *viewer = G4VVisManager::GetConcreteInstance();
  if (!viewer)
    return;
  G4VisAttributes style(G4Colour(0.15, 0.25, 0.95));
  style.SetLineWidth(3.0);
  G4Polyline outline;
  for (const auto &corner : plane_.Corners())
    outline.push_back(corner);
  outline.push_back(plane_.Corners().front());
  outline.SetVisAttributes(style);
  viewer->Draw(outline);
}
