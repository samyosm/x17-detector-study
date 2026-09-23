#include "physics/sources/MixedSource.hh"

#include "Randomize.hh"

#include <stdexcept>

MixedSource::MixedSource(double transitionEnergy, double x17Mass,
                         double x17Fraction)
    : x17Fraction_(x17Fraction), ipc_(transitionEnergy),
      x17_(transitionEnergy, x17Mass), selected_(nullptr) {
  if (!(x17Fraction_ > 0.0 && x17Fraction_ < 1.0)) {
    throw std::invalid_argument(
        "Mixed-source X17 fraction must be between 0 and 1");
  }
}

void MixedSource::Generate(G4Event *event) {
  selected_ = G4UniformRand() < x17Fraction_
                  ? static_cast<PrimarySource *>(&x17_)
                  : static_cast<PrimarySource *>(&ipc_);
  selected_->Generate(event);
}

int MixedSource::ModeId() const {
  if (!selected_)
    throw std::logic_error("Mixed-source mode queried before generation");
  return selected_->ModeId();
}
