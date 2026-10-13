#pragma once
#include "G4GDMLParser.hh"
#include "G4VUserDetectorConstruction.hh"
#include "configuration/Configuration.hh"
#include "geometry/SourceCapsule.hh"
#include <memory>
#include <string>

class DetectorConstruction final : public G4VUserDetectorConstruction {
public:
    DetectorConstruction(std::string path, Configuration config);
    G4VPhysicalVolume* Construct() override;
    void MoveSourceCapsule(const G4ThreeVector& position);
private:
    std::string geometryPath_;
    Configuration config_;
    G4GDMLParser parser_;
    std::unique_ptr<SourceCapsule> capsule_;
};
