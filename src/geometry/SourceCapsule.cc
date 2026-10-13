#include "geometry/SourceCapsule.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4ProductionCuts.hh"
#include "G4Region.hh"
#include "G4Tubs.hh"
#include "G4VisAttributes.hh"
#include "geometry/SourcePose.hh"
#include <cmath>
#include <stdexcept>

namespace {
constexpr double envelopeMargin = 1e-3;

G4Material *FindMaterial(const std::string &name) {
  auto *material = G4NistManager::Instance()->FindOrBuildMaterial(name);
  if (!material)
    throw std::runtime_error("Unknown capsule material: " + name);
  return material;
}

G4LogicalVolume *MakeTube(const std::string &name, const std::string &material,
                          double radius, double length) {
  return new G4LogicalVolume(
      new G4Tubs(name, 0, radius, length / 2, 0, CLHEP::twopi),
      FindMaterial(material), name);
}
} // namespace

G4RotationMatrix RotationToward(const G4ThreeVector &direction) {
  G4RotationMatrix rotation;
  const G4ThreeVector axis = G4ThreeVector(0, 0, 1).cross(direction);
  if (axis.mag() > 1e-12)
    rotation.rotate(std::acos(direction.z()), axis.unit());
  else if (direction.z() < 0)
    rotation.rotateX(CLHEP::pi);
  return rotation;
}

SourceCapsule::SourceCapsule(const CapsuleSpecification &capsule,
                             const PlacementSettings &placement,
                             G4LogicalVolume &world, double productionCut)
    : rotation_(RotationToward(placement.orientation)) {
  const double floor = -capsule.activeThickness / 2;
  const double front = floor + capsule.cavityDepth;
  const double back = front - capsule.bodyLength;
  const double end = front + capsule.windowThickness;
  auto *envelope = MakeTube("SourceCapsule", "G4_AIR",
                            capsule.bodyRadius + envelopeMargin,
                            end - back + 2 * envelopeMargin);
  auto *body = MakeTube("SourceBody", capsule.bodyMaterial, capsule.bodyRadius,
                        capsule.bodyLength);
  auto *cavity = MakeTube("SourceCavity", capsule.cavityMaterial,
                          capsule.cavityRadius, capsule.cavityDepth);
  auto *active = MakeTube("SourceActive", capsule.activeMaterial,
                          capsule.activeRadius, capsule.activeThickness);
  auto *window = MakeTube("SourceWindow", capsule.windowMaterial,
                          capsule.bodyRadius, capsule.windowThickness);
  const double envelopeCenter = (end + back) / 2;
  auto place = [](G4LogicalVolume *volume, G4LogicalVolume *mother, double z) {
    new G4PVPlacement(nullptr, {0, 0, z}, volume, volume->GetName(), mother,
                      false, 0, true);
  };
  place(body, envelope, back + capsule.bodyLength / 2 - envelopeCenter);
  place(window, envelope, front + capsule.windowThickness / 2 - envelopeCenter);
  place(cavity, body, front - capsule.cavityDepth / 2 - (back + capsule.bodyLength / 2));
  place(active, cavity, 0 - (front - capsule.cavityDepth / 2));
  auto *region = new G4Region("SourceCapsule");
  auto *cuts = new G4ProductionCuts;
  cuts->SetProductionCut(productionCut);
  region->SetProductionCuts(cuts);
  region->AddRootLogicalVolume(envelope);
  const G4ThreeVector start(placement.x, placement.y,
                            placement.uniformZ
                                ? (placement.zMinimum + placement.zMaximum) / 2
                                : placement.zFixed);
  const G4ThreeVector shift = rotation_ * G4ThreeVector(0, 0, envelopeCenter);
  placement_ = new G4PVPlacement(G4Transform3D(rotation_, start + shift), envelope,
                                 "SourceCapsule", &world, false, 0, true);
  envelopeOffset_ = shift;
  SourcePose::Instance().Set(start, rotation_);
}

void SourceCapsule::MoveTo(const G4ThreeVector &position) {
  placement_->SetTranslation(position + envelopeOffset_);
  if (placement_->CheckOverlaps(1000, 0, false, 1))
    throw std::runtime_error("Source capsule overlaps the detector at z = " +
                             std::to_string(position.z() / CLHEP::cm) + " cm");
  SourcePose::Instance().Set(position, rotation_);
}
