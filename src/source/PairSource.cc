#include "PairSource.hh"

#include "G4Electron.hh"
#include "G4Event.hh"
#include "G4IonTable.hh"
#include "G4ParticleTable.hh"
#include "G4PhysicalConstants.hh"
#include "G4Positron.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "Randomize.hh"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

G4ThreeVector IsotropicDirection() {
  // Uniform cos(theta) and azimuth give uniform solid angle.
  const double cosine = 2.0 * G4UniformRand() - 1.0;
  const double sine = std::sqrt(1.0 - cosine * cosine);
  const double phi = twopi * G4UniformRand();
  return {sine * std::cos(phi), sine * std::sin(phi), cosine};
}

G4ThreeVector BoostMomentum(const G4ThreeVector &restMomentum,
                            double restEnergy, const G4ThreeVector &direction,
                            double beta, double gamma) {
  // Boost only the component parallel to the moving pair's direction.
  const double parallel = restMomentum.dot(direction);
  return restMomentum +
         ((gamma - 1.0) * parallel + gamma * beta * restEnergy) * direction;
}

struct FinalState {
  G4ThreeVector positron;
  G4ThreeVector electron;
  G4ThreeVector recoil;
  double positronEnergy;
  double electronEnergy;
  double recoilEnergy;
};

FinalState DecayPair(double pairMass, double pairMomentum,
                     double daughterMass) {
  const double pairEnergy = std::hypot(pairMass, pairMomentum);
  const double beta = pairMomentum / pairEnergy;
  const double gamma = pairEnergy / pairMass;
  // Equal lepton energies in the pair rest frame follow from two-body decay.
  const double leptonEnergy = pairMass / 2.0;
  const double leptonMomentum = std::sqrt(leptonEnergy * leptonEnergy -
                                          electron_mass_c2 * electron_mass_c2);
  const auto direction = IsotropicDirection();
  const auto positronRest = leptonMomentum * IsotropicDirection();
  return {BoostMomentum(positronRest, leptonEnergy, direction, beta, gamma),
          BoostMomentum(-positronRest, leptonEnergy, direction, beta, gamma),
          -pairMomentum * direction,
          gamma * (leptonEnergy + beta * positronRest.dot(direction)),
          gamma * (leptonEnergy - beta * positronRest.dot(direction)),
          std::hypot(daughterMass, pairMomentum)};
}

void CheckConservation(const FinalState &state, double parentMass) {
  // The excited 8Be parent is assumed at rest; total p=0 and E=parent mass.
  if (std::abs(state.positronEnergy + state.electronEnergy +
               state.recoilEnergy - parentMass) > 1e-6 * MeV ||
      (state.positron + state.electron + state.recoil).mag() > 1e-6 * MeV) {
    throw std::runtime_error("Generated pair does not conserve four-momentum");
  }
}

void AddVertex(G4Event *event, G4ParticleDefinition *recoil,
               const FinalState &state) {
  // Match the reference source origin; its LiF target is separately at z=-0.5
  // mm.
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

double PairSource::TwoBodyMomentum(double parentMass, double daughterMass,
                                   double pairMass) {
  // Kallen-function form of the recoil momentum in a two-body decay.
  const double first = parentMass * parentMass -
                       (daughterMass + pairMass) * (daughterMass + pairMass);
  const double second = parentMass * parentMass -
                        (daughterMass - pairMass) * (daughterMass - pairMass);
  return std::sqrt(std::max(0.0, first * second)) / (2.0 * parentMass);
}

void PairSource::Generate(G4Event *event) {
  auto *recoil =
      G4ParticleTable::GetParticleTable()->GetIonTable()->GetIon(4, 8);
  const double daughterMass = recoil->GetPDGMass();
  const double parentMass = daughterMass + transitionEnergy_;
  const double pairMass = SamplePairMass(parentMass, daughterMass);
  const double pairMomentum =
      TwoBodyMomentum(parentMass, daughterMass, pairMass);
  const auto state = DecayPair(pairMass, pairMomentum, daughterMass);
  CheckConservation(state, parentMass);
  AddVertex(event, recoil, state);
}
