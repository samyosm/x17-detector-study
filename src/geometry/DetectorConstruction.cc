#include "geometry/DetectorConstruction.hh"
#include "G4NistManager.hh"
#include "geometry/CosmicSourcePlane.hh"
#include "physics/OpticalMaterials.hh"
#include "visualization/DetectorStyle.hh"

#include <stdexcept>
#include <utility>

DetectorConstruction::DetectorConstruction(std::string path,
                                           Configuration config)
    : geometryPath_(std::move(path)), config_(config) {}

G4VPhysicalVolume *DetectorConstruction::Construct() {
  for (const auto *name :
       {"G4_AIR", "G4_GRAPHITE", "G4_POLYSTYRENE", "G4_Cu", "G4_BRASS", "G4_Al",
        "G4_LITHIUM_FLUORIDE", "G4_POLYVINYL_CHLORIDE", "G4_CARBON_DIOXIDE"}) {
    G4NistManager::Instance()->FindOrBuildMaterial(name);
  }
  parser_.SetOverlapCheck(true);
  parser_.Read(geometryPath_);
  auto *scintillator = parser_.GetVolume("Scintillator");
  auto *world = parser_.GetVolume("World");
  if (!scintillator || !world) {
    throw std::runtime_error("GDML needs Scintillator and World volumes");
  }

  if (config_.Get<bool>("source.cosmic_muons.enable", false))
    CosmicSourcePlane(config_).RequireInside(*world->GetSolid());
  ApplyReferenceVisualization();
  if (!config_.Get<bool>("optics.enable", false))
    return parser_.GetWorldVolume();

  ConfigureOpticalMaterials(config_, *scintillator->GetMaterial(),
                            *world->GetMaterial());
  return parser_.GetWorldVolume();
}
