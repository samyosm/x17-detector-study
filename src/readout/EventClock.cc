#include "readout/EventClock.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"

void EventClock::Reset() { creationTimes_.clear(); }

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
  // When the optical processes are registered, an at-rest process that acts after the decay overwrites
  // the post-step time, losing the decay delay; the decay products still carry the right global time.
  const bool decayAtRest = step.GetPostStepPoint()->GetStepStatus() == fAtRestDoItProc;
  for (const auto *secondary : *secondaries) {
    double creation;
    if (primaryDecay)
      creation = 0.0;
    else if (decayAtRest)
      creation = CreationTime(track) + step.GetPreStepPoint()->GetLocalTime() +
                 (secondary->GetGlobalTime() - step.GetPreStepPoint()->GetGlobalTime());
    else
      creation = CreationTime(track) + step.GetPostStepPoint()->GetLocalTime();
    creationTimes_[secondary] = creation;
  }
}

double EventClock::StepTime(const G4Step &step) const {
  return CreationTime(step.GetTrack()) +
         (step.GetPreStepPoint()->GetLocalTime() +
          step.GetPostStepPoint()->GetLocalTime()) / 2;
}
