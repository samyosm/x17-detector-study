#include "application/actions/ActionInitialization.hh"
#include "output/RunFiles.hh"
#include "physics/OpticalMaterials.hh"
#include "application/actions/EventAction.hh"
#include "application/actions/RunAction.hh"
#include "application/actions/SteppingAction.hh"
#include "application/actions/PrimaryGeneratorAction.hh"

void ActionInitialization::BuildForMaster() const {
  SetUserAction(new RunAction(config_));
}

void ActionInitialization::Build() const {
  SetUserAction(new RunAction(config_));
  auto *source = new PrimaryGeneratorAction(config_);
  SetUserAction(source);
  auto *eventAction = new EventAction(source, config_);
  SetUserAction(eventAction);
  const auto efficiency =
      TracksOpticalPhotons(config_)
          ? (FoldsEfficiencyIntoYield(config_) ? 1.0 : config_.Get<double>("optics.sensor.quantum_efficiency"))
          : 0.0;
  SetUserAction(new SteppingAction(eventAction, efficiency));
}
