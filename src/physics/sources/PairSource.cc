#include "physics/sources/PairSource.hh"
#include "physics/PairDecay.hh"

#include "G4Electron.hh"
#include "G4Event.hh"
#include "G4IonTable.hh"
#include "G4ParticleTable.hh"
#include "G4PhysicalConstants.hh"
#include "G4Positron.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4ThreeVector.hh"

#include <stdexcept>

namespace {
void AttachDecayProductsToEvent(G4Event *event, G4ParticleDefinition *recoil,
                                const PairDecayProducts &state) {
  auto *vertex = new G4PrimaryVertex(G4ThreeVector(0, 0, 0), 0.0);
  vertex->SetPrimary(
      new G4PrimaryParticle(G4Positron::Definition(), state.positron.x(),
                            state.positron.y(), state.positron.z()));
  vertex->SetPrimary(
      new G4PrimaryParticle(G4Electron::Definition(), state.electron.x(),
                            state.electron.y(), state.electron.z()));
  vertex->SetPrimary(new G4PrimaryParticle(recoil, state.recoil.x(),
                                           state.recoil.y(), state.recoil.z()));
  event->AddPrimaryVertex(vertex);
}

} // namespace

PairSource::PairSource(double transitionEnergy)
    : transitionEnergy_(transitionEnergy) {
  if (transitionEnergy_ <= 2.0 * electron_mass_c2) {
    throw std::invalid_argument(
        "Transition energy must exceed two electron masses");
  }
}

void PairSource::Generate(G4Event *event) {
  auto *recoil =
      G4ParticleTable::GetParticleTable()->GetIonTable()->GetIon(4, 8);
  const double daughterMass = recoil->GetPDGMass();
  const double parentMass = daughterMass + transitionEnergy_;
  const double pairMass = SamplePairMass(parentMass, daughterMass);
  const double pairMomentum =
      CalculateTwoBodyMomentum(parentMass, daughterMass, pairMass);
  const auto state = SamplePairDecay(pairMass, pairMomentum, daughterMass);
  RequireFourMomentumConservation(state, parentMass);
  AttachDecayProductsToEvent(event, recoil, state);
}
