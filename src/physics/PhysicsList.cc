#include "physics/PhysicsList.hh"
#include "FTFP_BERT.hh"
#include "G4EmParameters.hh"
#include "G4EmStandardPhysics_option3.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4HadronicParameters.hh"
#include "G4OpticalParameters.hh"
#include "G4OpticalPhysics.hh"
#include "physics/OpticalMaterials.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4SystemOfUnits.hh"
#include "configuration/Configuration.hh"
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
std::string ElectromagneticOption(const Configuration &config) {
  const auto option = config.Get<std::string>("physics.em_option", "option3");
  if (option != "option3" && option != "option4")
    throw std::runtime_error("physics.em_option must be option3 or option4");
  return option;
}
} // namespace

G4VModularPhysicsList *CreatePhysicsList(const Configuration &config) {
  auto *physics = new FTFP_BERT;
  const bool lab = config.Get<bool>("source.lab.enable", false);
  if (lab && ElectromagneticOption(config) == "option4")
    physics->ReplacePhysics(new G4EmStandardPhysics_option4);
  else if (lab || config.Get<bool>("source.cosmic_muons.enable", false))
    physics->ReplacePhysics(new G4EmStandardPhysics_option3);
  if (lab) {
    physics->SetDefaultCutValue(config.Get<double>("physics.default_cut_mm") * mm);
    auto *em = G4EmParameters::Instance();
    const bool atomic = config.Get<bool>("physics.atomic_deexcitation");
    em->SetFluo(atomic);
    em->SetAuger(atomic);
    em->SetDeexcitationIgnoreCut(atomic);
    G4HadronicParameters::Instance()->SetTimeThresholdForRadioactiveDecay(
        std::numeric_limits<double>::max());
    physics->RegisterPhysics(new G4RadioactiveDecayPhysics);
  }
  if (TracksOpticalPhotons(config))
    physics->RegisterPhysics(new G4OpticalPhysics);
  G4OpticalParameters::Instance()->SetProcessActivation("Cerenkov", false);
  return physics;
}

std::string DescribePhysics(const Configuration &config) {
  const bool lab = config.Get<bool>("source.lab.enable", false);
  std::ostringstream text;
  text << "FTFP_BERT + G4EmStandardPhysics_"
       << (lab ? ElectromagneticOption(config) : "option3");
  if (lab)
    text << " + G4RadioactiveDecayPhysics; default production cut "
         << config.Get<double>("physics.default_cut_mm") << " mm";
  if (lab && config.Get<bool>("source.lab.encapsulated", true))
    text << "; source capsule cut " << config.Get<double>("physics.capsule_cut_mm") << " mm";
  if (lab)
    text << "; atomic de-excitation (fluorescence, Auger) "
         << (config.Get<bool>("physics.atomic_deexcitation") ? "on" : "off");
  if (TracksOpticalPhotons(config))
    text << " + G4OpticalPhysics (scintillation, absorption, boundary; Cerenkov off)";
  return text.str();
}
