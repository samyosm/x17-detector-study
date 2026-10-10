#include "physics/PhysicsList.hh"
#include "FTFP_BERT.hh"
#include "G4EmStandardPhysics_option3.hh"
#include "G4HadronicParameters.hh"
#include "G4OpticalParameters.hh"
#include "G4OpticalPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "configuration/Configuration.hh"
#include <limits>

G4VModularPhysicsList *CreatePhysicsList(const Configuration &config) {
  auto *physics = new FTFP_BERT;
  const bool lab = config.Get<bool>("source.lab.enable", false);
  if (lab || config.Get<bool>("source.cosmic_muons.enable", false))
    physics->ReplacePhysics(new G4EmStandardPhysics_option3);
  if (lab) {
    G4HadronicParameters::Instance()->SetTimeThresholdForRadioactiveDecay(
        std::numeric_limits<double>::max());
    physics->RegisterPhysics(new G4RadioactiveDecayPhysics);
  }
  if (config.Get<bool>("optics.enable", false))
    physics->RegisterPhysics(new G4OpticalPhysics);
  G4OpticalParameters::Instance()->SetProcessActivation("Cerenkov", false);
  return physics;
}
