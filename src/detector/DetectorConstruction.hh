#pragma once
#include "G4GDMLParser.hh"
#include "G4VUserDetectorConstruction.hh"
#include <string>

class DetectorConstruction final : public G4VUserDetectorConstruction {
public:
    explicit DetectorConstruction(std::string path);
    G4VPhysicalVolume* Construct() override;
private:
    std::string path_;
    G4GDMLParser parser_;
};
