#include "DetectorConstruction.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4SystemOfUnits.hh"

#include <stdexcept>
#include <utility>
#include <vector>

DetectorConstruction::DetectorConstruction(std::string path, OpticalConfig optics)
    : path_(std::move(path)), optics_(optics) {}

G4VPhysicalVolume* DetectorConstruction::Construct() {
    parser_.Read(path_);
    auto* scintillator = parser_.GetVolume("Scintillator");
    auto* world = parser_.GetVolume("World");
    auto* pmt = parser_.GetVolume("PMT");
    if (!scintillator || !world || !pmt) {
        throw std::runtime_error("GDML needs Scintillator, World, and PMT volumes");
    }

    const std::vector<G4double> energies = {
        optics_.emissionMinEv * eV, optics_.emissionMaxEv * eV};
    const std::vector<G4double> flat = {1.0, 1.0};
    auto* scintillation = new G4MaterialPropertiesTable;
    scintillation->AddProperty("RINDEX", energies,
                              {optics_.scintillatorIndex, optics_.scintillatorIndex});
    scintillation->AddProperty("ABSLENGTH", energies,
                              {optics_.scintillatorAbsorptionCm * cm,
                               optics_.scintillatorAbsorptionCm * cm});
    scintillation->AddProperty("SCINTILLATIONCOMPONENT1", energies, flat);
    scintillation->AddConstProperty("SCINTILLATIONYIELD", optics_.yieldPerMeV / MeV);
    scintillation->AddConstProperty("RESOLUTIONSCALE", optics_.resolutionScale);
    scintillation->AddConstProperty("SCINTILLATIONTIMECONSTANT1",
                                     optics_.decayTimeNs * ns);
    scintillation->AddConstProperty("SCINTILLATIONYIELD1", 1.0);
    scintillator->GetMaterial()->SetMaterialPropertiesTable(scintillation);

    auto* air = new G4MaterialPropertiesTable;
    air->AddProperty("RINDEX", energies, {optics_.airIndex, optics_.airIndex});
    world->GetMaterial()->SetMaterialPropertiesTable(air);
    auto* glass = new G4MaterialPropertiesTable;
    glass->AddProperty("RINDEX", energies, {optics_.glassIndex, optics_.glassIndex});
    pmt->GetMaterial()->SetMaterialPropertiesTable(glass);
    return parser_.GetWorldVolume();
}
