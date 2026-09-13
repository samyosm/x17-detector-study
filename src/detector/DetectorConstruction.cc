#include "DetectorConstruction.hh"
#include <utility>

DetectorConstruction::DetectorConstruction(std::string path)
    : path_(std::move(path)) {}

G4VPhysicalVolume* DetectorConstruction::Construct() {
    parser_.Read(path_);
    return parser_.GetWorldVolume();
}
