#include "ActionInitialization.hh"
#include "EventAction.hh"
#include "source/PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"

#include <utility>

ActionInitialization::ActionInitialization(
    std::shared_ptr<const SourceSettings> settings, std::string outputFile,
    int progressInterval, double quantumEfficiency)
    : settings_(std::move(settings)), outputFile_(std::move(outputFile)),
      progressInterval_(progressInterval), quantumEfficiency_(quantumEfficiency) {}

void ActionInitialization::BuildForMaster() const {
    SetUserAction(new RunAction(outputFile_));
}

void ActionInitialization::Build() const {
    SetUserAction(new RunAction(outputFile_));
    auto* source = new PrimaryGeneratorAction(*settings_);
    SetUserAction(source);
    auto* eventAction = new EventAction(source, progressInterval_);
    SetUserAction(eventAction);
    SetUserAction(new SteppingAction(eventAction, quantumEfficiency_));
}
