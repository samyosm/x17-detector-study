#include "readout/BarHitReadout.hh"
#include "G4EmSaturation.hh"
#include "G4LossTableManager.hh"
#include "G4OpticalPhoton.hh"
#include "G4Step.hh"
#include "G4TouchableHistory.hh"
#include "physics/OpticalMaterials.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include <algorithm>

BarHitReadout::BarHitReadout(const Configuration &config)
    : window_(config.Get<double>("readout.raw_hits.window_ns") * ns) {
  if (UsesLightMap(config)) {
    lightMap_ = &LightMap::Shared(config);
    saturation_ = G4LossTableManager::Instance()->EmSaturation();
    shape_ = FindBarShape();
    detectedPhotonsPerMeV_ = config.Get<double>("optics.scintillator.yield_per_mev") *
                             config.Get<double>("optics.sensor.quantum_efficiency");
  }
}

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
  std::array<double, 2> photons{0.0, 0.0};
  if (lightMap_) {
    const auto middle = (before.GetPosition() + after.GetPosition()) / 2;
    const auto local = before.GetTouchableHandle()->GetHistory()->GetTopTransform().TransformPoint(middle);
    const auto collection = lightMap_->Collection(shape_, local);
    // Same visible (Birks-quenched) energy that the scintillation process turns into photons.
    const double visible = saturation_ ? saturation_->VisibleEnergyDepositionAtAStep(&step) : energy;
    for (int end = 0; end < 2; ++end)
      photons[end] = visible / MeV * detectedPhotonsPerMeV_ * collection[end];
  }
  deposits_[armIndex].push_back(
      {time, energy, (before.GetPosition().z() + after.GetPosition().z()) / 2, photons});
}

std::vector<BarHit> BarHitReadout::Hits() {
  std::vector<BarHit> hits;
  for (int arm = 0; arm < detector::armCount; ++arm) {
    auto &deposits = deposits_[arm];
    std::sort(deposits.begin(), deposits.end(),
              [](const Deposit &a, const Deposit &b) { return a.time < b.time; });
    for (std::size_t first = 0, last; first < deposits.size(); first = last) {
      BarHit hit{arm + 1, 0, 0, 0, deposits[first].time};
      for (last = first; last < deposits.size() &&
                         deposits[last].time - deposits[first].time <= window_;
           ++last) {
        hit.energy += deposits[last].energy;
        hit.position += deposits[last].energy * deposits[last].position;
        hit.time += deposits[last].energy * deposits[last].time;
        for (int end = 0; end < 2; ++end)
          hit.photons[end] += deposits[last].photons[end];
      }
      hit.position /= hit.energy;
      hit.time /= hit.energy;
      hits.push_back(hit);
    }
  }
  return hits;
}
