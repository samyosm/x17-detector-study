#pragma once
#include "G4GDMLParser.hh"
#include "G4VUserDetectorConstruction.hh"
#include "configuration/Configuration.hh"
#include <string>

class DetectorConstruction final : public G4VUserDetectorConstruction {
public:
    DetectorConstruction(std::string path, OpticalConfig optics);
    G4VPhysicalVolume* Construct() override;
private:
    std::string path_;
    OpticalConfig optics_;
    G4GDMLParser parser_;
};
