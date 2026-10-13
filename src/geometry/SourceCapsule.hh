#pragma once
#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"
#include "physics/LabSourceDefinition.hh"

class G4LogicalVolume;
class G4PVPlacement;

class SourceCapsule {
public:
  SourceCapsule(const CapsuleSpecification &specification,
                const PlacementSettings &placement, G4LogicalVolume &world,
                double productionCut);
  void MoveTo(const G4ThreeVector &position);

private:
  G4PVPlacement *placement_;
  G4RotationMatrix rotation_;
  G4ThreeVector envelopeOffset_;
};

G4RotationMatrix RotationToward(const G4ThreeVector &direction);
