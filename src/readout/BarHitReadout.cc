#include "readout/BarHitReadout.hh"
#include "G4OpticalPhoton.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include <algorithm>

BarHitReadout::BarHitReadout(const Configuration &config)
    : window_(config.Get<double>("readout.raw_hits.window_ns") * ns) {}

void BarHitReadout::Reset() {
  for (auto &deposits : deposits_)
    deposits.clear();
}

void BarHitReadout::RecordDeposit(const G4Step &step, int armIndex, double time) {
  const double energy = step.GetTotalEnergyDeposit();
  if (energy <= 0 || step.GetTrack()->GetDefinition() == G4OpticalPhoton::Definition())
    return;
  const auto &before = *step.GetPreStepPoint();
  const auto &after = *step.GetPostStepPoint();
  deposits_[armIndex].push_back(
      {time, energy, (before.GetPosition().z() + after.GetPosition().z()) / 2});
}

std::vector<BarHit> BarHitReadout::Hits() {
  std::vector<BarHit> hits;
  for (int arm = 0; arm < detector::armCount; ++arm) {
    auto &deposits = deposits_[arm];
    std::sort(deposits.begin(), deposits.end(),
              [](const Deposit &a, const Deposit &b) { return a.time < b.time; });
    for (std::size_t first = 0, last; first < deposits.size(); first = last) {
      BarHit hit{arm + 1, 0, 0, 0};
      for (last = first; last < deposits.size() &&
                         (last == first || deposits[last].time - deposits[last - 1].time <= window_);
           ++last) {
        hit.energy += deposits[last].energy;
        hit.position += deposits[last].energy * deposits[last].position;
        hit.time += deposits[last].energy * deposits[last].time;
      }
      hit.position /= hit.energy;
      hit.time /= hit.energy;
      hits.push_back(hit);
    }
  }
  return hits;
}
