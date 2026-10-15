#pragma once
#include <unordered_map>

class G4Step;
class G4Track;

class EventClock {
public:
  void Reset();
  void RegisterSecondaries(const G4Step &step);
  double StepTime(const G4Step &step) const;

private:
  double CreationTime(const G4Track *track) const;
  std::unordered_map<const G4Track *, double> creationTimes_;
};
