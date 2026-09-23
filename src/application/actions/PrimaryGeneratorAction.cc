#include "application/actions/PrimaryGeneratorAction.hh"
#include "physics/sources/SourceFactory.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction(const Configuration &settings)
    : source_(CreatePrimarySource(settings)) {}

PrimaryGeneratorAction::~PrimaryGeneratorAction() = default;

void PrimaryGeneratorAction::GeneratePrimaries(G4Event *event) {
  source_->Generate(event);
}
