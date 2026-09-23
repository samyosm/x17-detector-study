#include "physics/OpticalMaterials.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4SystemOfUnits.hh"
#include "configuration/Configuration.hh"
#include <vector>

void ConfigureOpticalMaterials(const Configuration &config,
                               G4Material &scintillatorMaterial,
                               G4Material &airMaterial) {
  const std::vector<G4double> energies = {
      config.Get<double>("optics.scintillator.emission_min_ev") * eV,
      config.Get<double>("optics.scintillator.emission_max_ev") * eV};
  const std::vector<G4double> flat = {1.0, 1.0};
  auto *scintillation = new G4MaterialPropertiesTable;
  scintillation->AddProperty(
      "RINDEX", energies,
      {config.Get<double>("optics.scintillator.refractive_index"),
       config.Get<double>("optics.scintillator.refractive_index")});
  scintillation->AddProperty(
      "ABSLENGTH", energies,
      {config.Get<double>("optics.scintillator.absorption_length_cm") * cm,
       config.Get<double>("optics.scintillator.absorption_length_cm") * cm});
  scintillation->AddProperty("SCINTILLATIONCOMPONENT1", energies, flat);
  scintillation->AddConstProperty(
      "SCINTILLATIONYIELD",
      config.Get<double>("optics.scintillator.yield_per_mev") / MeV);
  scintillation->AddConstProperty(
      "RESOLUTIONSCALE",
      config.Get<double>("optics.scintillator.resolution_scale"));
  scintillation->AddConstProperty(
      "SCINTILLATIONTIMECONSTANT1",
      config.Get<double>("optics.scintillator.decay_ns") * ns);
  scintillation->AddConstProperty("SCINTILLATIONYIELD1", 1.0);
  scintillatorMaterial.SetMaterialPropertiesTable(scintillation);

  auto *air = new G4MaterialPropertiesTable;
  air->AddProperty("RINDEX", energies,
                   {config.Get<double>("optics.air.refractive_index"),
                    config.Get<double>("optics.air.refractive_index")});
  airMaterial.SetMaterialPropertiesTable(air);
}
