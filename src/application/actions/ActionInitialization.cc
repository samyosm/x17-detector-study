#include "application/actions/ActionInitialization.hh"
#include "output/RunFiles.hh"
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
  auto *eventAction = new EventAction(
      source, config_.Get<int>("runtime.progress_interval"),
      config_.Get<bool>("source.cosmic_muons.enable", false) ? &config_
                                                             : nullptr,
      UsesCheckpoints(config_) ? CheckpointEventCount(config_) : 0);
  SetUserAction(eventAction);
  const auto efficiency =
      config_.Get<bool>("optics.enable", false)
          ? config_.Get<double>("optics.sensor.quantum_efficiency")
          : 0.0;
  SetUserAction(new SteppingAction(eventAction, efficiency));
}
