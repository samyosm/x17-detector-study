#include "RunAction.hh"

#include "G4AnalysisManager.hh"

#include <iomanip>
#include <sstream>
#include <utility>

RunAction::RunAction(std::string outputFile)
    : outputFile_(std::move(outputFile)) {
    auto* analysis = G4AnalysisManager::Instance();
    analysis->SetNtupleMerging(true);
    analysis->SetNtupleRowWise(false, true);
    analysis->CreateNtuple("events", "Scintillator energy, PMT photons, and pair truth");
    analysis->CreateNtupleIColumn("event_id");
    analysis->CreateNtupleIColumn("source_mode");
    analysis->CreateNtupleDColumn("pair_mass_MeV");
    analysis->CreateNtupleDColumn("opening_angle_deg");
    analysis->CreateNtupleDColumn("positron_energy_MeV");
    analysis->CreateNtupleDColumn("electron_energy_MeV");
    for (int axis = 0; axis < 3; ++axis) {
        const char* coordinate = axis == 0 ? "x" : axis == 1 ? "y" : "z";
        analysis->CreateNtupleDColumn(std::string("positron_p") + coordinate + "_MeV");
        analysis->CreateNtupleDColumn(std::string("electron_p") + coordinate + "_MeV");
    }
    for (int scintillator = 1; scintillator <= 16; ++scintillator) {
        std::ostringstream name;
        name << "scint_" << std::setw(2) << std::setfill('0') << scintillator
             << "_edep_MeV";
        analysis->CreateNtupleDColumn(name.str());
    }
    for (int scintillator = 1; scintillator <= 16; ++scintillator) {
        for (const char* end : {"D", "U"}) {
            std::ostringstream name;
            name << "pmt_" << std::setw(2) << std::setfill('0') << scintillator
                 << '_' << end;
            analysis->CreateNtupleIColumn(name.str() + "_photons");
            analysis->CreateNtupleDColumn(name.str() + "_first_time_ns");
        }
    }
    analysis->FinishNtuple();
    analysis->CreateNtuple("photon_hits", "Detected optical photon arrival times");
    analysis->CreateNtupleIColumn("event_id");
    analysis->CreateNtupleIColumn("channel");
    analysis->CreateNtupleDColumn("time_ns");
    analysis->CreateNtupleDColumn("energy_eV");
    analysis->FinishNtuple();
}

void RunAction::BeginOfRunAction(const G4Run*) {
    G4AnalysisManager::Instance()->OpenFile(outputFile_);
}

void RunAction::EndOfRunAction(const G4Run*) {
    auto* analysis = G4AnalysisManager::Instance();
    analysis->Write();
    analysis->CloseFile();
}
