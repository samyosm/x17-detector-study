#include "physics/PhysicsList.hh"
#include "FTFP_BERT.hh"
#include "G4EmStandardPhysics_option3.hh"
#include "G4OpticalParameters.hh"
#include "G4OpticalPhysics.hh"
#include "configuration/Configuration.hh"

G4VModularPhysicsList *CreatePhysicsList(const Configuration &config) {
  auto *physics = new FTFP_BERT;
  if (config.Get<bool>("source.cosmic_muons.enable", false))
    physics->ReplacePhysics(new G4EmStandardPhysics_option3);
  if (config.Get<bool>("optics.enable", false))
    physics->RegisterPhysics(new G4OpticalPhysics);
  G4OpticalParameters::Instance()->SetProcessActivation("Cerenkov", false);
  return physics;
}
