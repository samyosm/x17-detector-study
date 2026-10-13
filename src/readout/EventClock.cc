#include "readout/EventClock.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"

void EventClock::Reset() {
  creationTimes_.clear();
  origin_ = 0.0;
}

double EventClock::CreationTime(const G4Track *track) const {
  const auto found = creationTimes_.find(track);
  return found == creationTimes_.end() ? 0.0 : found->second;
}

void EventClock::RegisterSecondaries(const G4Step &step) {
  const auto *secondaries = step.GetSecondaryInCurrentStep();
  if (secondaries->empty())
    return;
  const auto *track = step.GetTrack();
  const auto *process = step.GetPostStepPoint()->GetProcessDefinedStep();
  const bool primaryDecay = track->GetParentID() == 0 && process &&
                            process->GetProcessName() == "RadioactiveDecay";
  if (primaryDecay)
    origin_ = step.GetPostStepPoint()->GetGlobalTime();
  const double creation =
      primaryDecay ? 0.0
                   : CreationTime(track) + step.GetPostStepPoint()->GetLocalTime();
  for (const auto *secondary : *secondaries)
    creationTimes_[secondary] = creation;
}

double EventClock::StepTime(const G4Step &step) const {
  return CreationTime(step.GetTrack()) +
         (step.GetPreStepPoint()->GetLocalTime() +
          step.GetPostStepPoint()->GetLocalTime()) / 2;
}
