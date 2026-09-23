#include "G4AnalysisManager.hh"
#include "output/RootOutput.hh"
#include <iomanip>
#include <sstream>

namespace {
void DefineCosmicArmTree() {
  auto *output = G4AnalysisManager::Instance();
  output->CreateNtuple(
      "cosmic_arms", "Deposits and centroids; only arms with nonzero deposits");
  for (auto name : {"event_id", "arm"})
    output->CreateNtupleIColumn(name);
  for (auto layer : {"e", "delta_e", "mwpc"}) {
    output->CreateNtupleDColumn(std::string(layer) + "_edep_MeV");
    for (auto axis : {"x", "y", "z"})
      output->CreateNtupleDColumn(std::string(layer) + "_" + axis + "_cm");
  }
  output->FinishNtuple();
}

void DefineCosmicEventTree() {
  auto *output = G4AnalysisManager::Instance();
  output->CreateNtuple(
      "cosmic_events",
      "Every incident muon, including misses; ideal deposit observables");
  for (auto name :
       {"event_id", "muon_pdg", "n_hit", "hit_mask", "arm_1", "arm_2"})
    output->CreateNtupleIColumn(name);
  for (auto name : {"muon_kinetic_MeV", "source_x_cm", "source_y_cm",
                    "source_z_cm", "direction_x", "direction_y", "direction_z",
                    "cos_zenith", "energy_sum_MeV", "energy_asymmetry",
                    "opening_angle_deg", "opening_angle_z25mm_deg"})
    output->CreateNtupleDColumn(name);
  output->FinishNtuple();
}

void DefineCosmicStepTree() {
  auto *output = G4AnalysisManager::Instance();
  output->CreateNtuple(
      "cosmic_steps",
      "Optional unsmeared detector deposits; layer 0=E,1=DeltaE,2=MWPC");
  for (auto name : {"event_id", "layer", "arm", "track_id", "parent_id", "pdg"})
    output->CreateNtupleIColumn(name);
  for (auto name : {"edep_MeV", "time_ns", "pre_x_cm", "pre_y_cm", "pre_z_cm",
                    "post_x_cm", "post_y_cm", "post_z_cm"})
    output->CreateNtupleDColumn(name);
  output->FinishNtuple();
}

void DefineEventTree() {
  auto *output = G4AnalysisManager::Instance();
  output->CreateNtuple("events",
                       "Scintillator energy, PMT photons, and pair truth");
  output->CreateNtupleIColumn("event_id");
  output->CreateNtupleIColumn("source_mode");
  output->CreateNtupleDColumn("pair_mass_MeV");
  output->CreateNtupleDColumn("opening_angle_deg");
  output->CreateNtupleDColumn("positron_energy_MeV");
  output->CreateNtupleDColumn("electron_energy_MeV");
  for (int axis = 0; axis < 3; ++axis) {
    const char *coordinate = axis == 0 ? "x" : axis == 1 ? "y" : "z";
    output->CreateNtupleDColumn(std::string("positron_p") + coordinate +
                                "_MeV");
    output->CreateNtupleDColumn(std::string("electron_p") + coordinate +
                                "_MeV");
  }
  for (int scintillator = 1; scintillator <= detector::armCount;
       ++scintillator) {
    std::ostringstream name;
    name << "scint_" << std::setw(2) << std::setfill('0') << scintillator
         << "_edep_MeV";
    output->CreateNtupleDColumn(name.str());
  }
  for (int scintillator = 1; scintillator <= detector::armCount;
       ++scintillator) {
    for (const char *end : {"D", "U"}) {
      std::ostringstream name;
      name << "pmt_" << std::setw(2) << std::setfill('0') << scintillator << '_'
           << end;
      output->CreateNtupleIColumn(name.str() + "_photons");
      output->CreateNtupleDColumn(name.str() + "_first_time_ns");
    }
  }
  output->FinishNtuple();
}

void DefinePhotonHitTree() {
  auto *output = G4AnalysisManager::Instance();
  output->CreateNtuple("photon_hits", "Detected optical photon arrival times");
  output->CreateNtupleIColumn("event_id");
  output->CreateNtupleIColumn("channel");
  output->CreateNtupleDColumn("time_ns");
  output->CreateNtupleDColumn("energy_eV");
  output->FinishNtuple();
}
} // namespace

void DefineRootTrees() {
  auto *output = G4AnalysisManager::Instance();
  output->SetNtupleMerging(true);
  output->SetNtupleRowWise(false, true);
  DefineEventTree();
  DefinePhotonHitTree();
  DefineCosmicArmTree();
  DefineCosmicEventTree();
  DefineCosmicStepTree();
}
