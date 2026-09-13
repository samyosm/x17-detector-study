#include "PrimaryGeneratorAction.hh"

#include "GunSource.hh"
#include "IpcSource.hh"
#include "X17Source.hh"

#include <stdexcept>

namespace {

std::unique_ptr<PrimarySource> MakeSource(const SourceSettings &settings) {
  if (settings.mode == "gun")
    return std::make_unique<GunSource>(settings);
  if (settings.mode == "ipc") {
    return std::make_unique<IpcSource>(settings.transitionEnergy);
  }
  if (settings.mode == "x17") {
    return std::make_unique<X17Source>(settings.transitionEnergy,
                                       settings.x17Mass);
  }
  throw std::invalid_argument("Unknown source mode: " + settings.mode);
}

} // namespace

PrimaryGeneratorAction::PrimaryGeneratorAction(const SourceSettings &settings)
    : source_(MakeSource(settings)) {}

PrimaryGeneratorAction::~PrimaryGeneratorAction() = default;

void PrimaryGeneratorAction::GeneratePrimaries(G4Event *event) {
  source_->Generate(event);
}
