#include "geometry/DetectorConstruction.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include "geometry/CosmicSourcePlane.hh"
#include "physics/LabSourceDefinition.hh"
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
  if (config_.Get<bool>("source.lab.enable", false) &&
      config_.Get<bool>("source.lab.encapsulated", true)) {
    const auto source = ReadLabSource(config_);
    capsule_ = std::make_unique<SourceCapsule>(
        source.capsule, source.placement, *world,
        config_.Get<double>("physics.capsule_cut_mm") * mm);
  }
  ApplyReferenceVisualization();
  if (!TracksOpticalPhotons(config_))
    return parser_.GetWorldVolume();

  ConfigureOpticalMaterials(config_, *scintillator->GetMaterial(),
                            *world->GetMaterial());
  ConfigureScintillatorWrap(config_, *scintillator);
  return parser_.GetWorldVolume();
}

void DetectorConstruction::MoveSourceCapsule(const G4ThreeVector &position) {
  if (capsule_)
    capsule_->MoveTo(position);
}
