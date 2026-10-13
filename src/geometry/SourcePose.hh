#pragma once
#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"

class SourcePose {
public:
  static SourcePose &Instance() {
    static SourcePose pose;
    return pose;
  }
  const G4ThreeVector &Position() const { return position_; }
  const G4RotationMatrix &Rotation() const { return rotation_; }
  void Set(const G4ThreeVector &position, const G4RotationMatrix &rotation) {
    position_ = position;
    rotation_ = rotation;
  }

private:
  G4ThreeVector position_;
  G4RotationMatrix rotation_;
};
