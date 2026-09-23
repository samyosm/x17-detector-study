#pragma once

#include "G4Colour.hh"
#include "G4LogicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4VisAttributes.hh"

// Active B4cDetectorConstruction.cc visualization attributes, including visible
// enclosing volumes. Hiding a mother for inspection is a view-only override.
inline void ApplyReferenceVisualization() {
  for (auto *volume : *G4LogicalVolumeStore::GetInstance()) {
    const auto &name = volume->GetName();
    if (name == "World" || name == "BeamlineVacuum") {
      volume->SetVisAttributes(G4VisAttributes::GetInvisible());
      continue;
    }
    G4Colour colour;
    if (name == "Scintillator" || name.rfind("DeltaEScintillator_", 0) == 0)
      colour = G4Colour::Cyan();
    else if (name == "BeamlineWall" || name == "ScintillatorWrap")
      colour = G4Colour::Green();
    else if (name == "MWPCInnerWall" || name == "MWPCOuterWall" ||
             name == "BrassNut")
      colour = G4Colour::Gray();
    else if (name == "MWPCGas")
      colour = G4Colour(0.9, 0.6, 0.4);
    else if (name.rfind("DeltaEHousing_", 0) == 0)
      colour = G4Colour::Black();
    else if (name.rfind("DeltaECavity_", 0) == 0)
      colour = G4Colour::White();
    else if (name == "LithiumFluorideTarget")
      colour = G4Colour(0.9, 0.77, 0.13);
    else if (name == "Flange")
      colour = G4Colour(0.67, 0.65, 0.61);
    else if (name == "CoolingRod")
      colour = G4Colour(0.64, 0.49, 0.06);
    else
      continue;
    volume->SetVisAttributes(G4VisAttributes(colour));
  }
}
