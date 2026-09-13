#include "actions/ActionInitialization.hh"
#include "configuration/Configuration.hh"
#include "detector/DetectorConstruction.hh"
#include "source/SourceSettings.hh"

#include "FTFP_BERT.hh"
#include "G4RunManagerFactory.hh"
#include "G4SystemOfUnits.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"

#include <exception>
#include <iostream>
#include <memory>

int main() {
    try {
        const auto config = LoadConfiguration();
        const auto& position = config.gunPositionCm;
        const auto& direction = config.gunDirection;
        auto source = std::make_shared<SourceSettings>(SourceSettings{
            config.sourceMode,
            config.transitionEnergyMeV * MeV,
            config.x17MassMeV * MeV,
            config.gunParticle,
            config.gunEnergyMeV * MeV,
            {position[0] * cm, position[1] * cm, position[2] * cm},
            {direction[0], direction[1], direction[2]}
        });

        auto runManager = std::unique_ptr<G4RunManager>(
            G4RunManagerFactory::CreateRunManager(G4RunManagerType::MT));
        runManager->SetNumberOfThreads(config.threads);
        runManager->SetUserInitialization(
            new DetectorConstruction(config.geometryFile.string()));
        runManager->SetUserInitialization(new FTFP_BERT);
        runManager->SetUserInitialization(
            new ActionInitialization(source, config.outputFile));
        runManager->Initialize();

        auto* ui = G4UImanager::GetUIpointer();
        if (config.mode == "batch") {
            return ui->ApplyCommand("/run/beamOn " + std::to_string(config.events)) == 0 ? 0 : 1;
        }

        G4VisExecutive visualization;
        visualization.Initialize();
        char name[] = "simulate_detection";
        char* arguments[] = {name, nullptr};
        G4UIExecutive session(1, arguments);
        if (ui->ApplyCommand("/control/execute " + config.visualizationMacro.string()) != 0) return 1;
        session.SessionStart();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
