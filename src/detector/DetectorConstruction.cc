#include "DetectorConstruction.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include "Visualization.hh"

#include <stdexcept>
#include <utility>
#include <vector>

DetectorConstruction::DetectorConstruction(std::string path,
                                           OpticalConfig optics)
    : path_(std::move(path)), optics_(optics) {}

G4VPhysicalVolume *DetectorConstruction::Construct() {
  // Resolve the same NIST materials used by the reference construction.
  for (const auto *name :
       {"G4_AIR", "G4_GRAPHITE", "G4_POLYSTYRENE", "G4_Cu", "G4_BRASS", "G4_Al",
        "G4_LITHIUM_FLUORIDE", "G4_POLYVINYL_CHLORIDE", "G4_CARBON_DIOXIDE"}) {
    G4NistManager::Instance()->FindOrBuildMaterial(name);
  }
  parser_.SetOverlapCheck(true);
  parser_.Read(path_);
  auto *scintillator = parser_.GetVolume("Scintillator");
  auto *world = parser_.GetVolume("World");
  if (!scintillator || !world) {
    throw std::runtime_error("GDML needs Scintillator and World volumes");
  }

  const std::vector<G4double> energies = {optics_.emissionMinEv * eV,
                                          optics_.emissionMaxEv * eV};
  const std::vector<G4double> flat = {1.0, 1.0};
  auto *scintillation = new G4MaterialPropertiesTable;
  scintillation->AddProperty(
      "RINDEX", energies,
      {optics_.scintillatorIndex, optics_.scintillatorIndex});
  scintillation->AddProperty("ABSLENGTH", energies,
                             {optics_.scintillatorAbsorptionCm * cm,
                              optics_.scintillatorAbsorptionCm * cm});
  scintillation->AddProperty("SCINTILLATIONCOMPONENT1", energies, flat);
  scintillation->AddConstProperty("SCINTILLATIONYIELD",
                                  optics_.yieldPerMeV / MeV);
  scintillation->AddConstProperty("RESOLUTIONSCALE", optics_.resolutionScale);
  scintillation->AddConstProperty("SCINTILLATIONTIMECONSTANT1",
                                  optics_.decayTimeNs * ns);
  scintillation->AddConstProperty("SCINTILLATIONYIELD1", 1.0);
  scintillator->GetMaterial()->SetMaterialPropertiesTable(scintillation);

  auto *air = new G4MaterialPropertiesTable;
  air->AddProperty("RINDEX", energies, {optics_.airIndex, optics_.airIndex});
  world->GetMaterial()->SetMaterialPropertiesTable(air);
  // PVC deliberately has no optical table: photons reaching its interfaces
  // are absorbed. End arrivals are observed by the virtual sensors in stepping.
  // Polystyrene is shared with the thin DeltaE scintillators, so those also
  // scintillate, but only the long bars feed our end readout.
  ApplyReferenceVisualization();
  return parser_.GetWorldVolume();
}
