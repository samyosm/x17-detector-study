#pragma once
#include <unordered_map>

class G4Step;
class G4Track;

class EventClock {
public:
  void Reset();
  void RegisterSecondaries(const G4Step &step);
  double StepTime(const G4Step &step) const;
  // Time since the primary decay for a global time (optical photons do not inherit the clock).
  double SinceDecay(double globalTime) const { return globalTime - origin_; }

private:
  double CreationTime(const G4Track *track) const;
  std::unordered_map<const G4Track *, double> creationTimes_;
  double origin_ = 0.0;
};
