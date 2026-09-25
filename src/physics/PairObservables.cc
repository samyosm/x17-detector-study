#include "physics/PairObservables.hh"
#include "G4Event.hh"
#include "G4PhysicalConstants.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"
#include <algorithm>
#include <cmath>

PairObservables CalculatePrimaryPairObservables(const G4Event &event) {
  PairObservables result;
  const G4PrimaryParticle *positron = nullptr;
  const G4PrimaryParticle *electron = nullptr;

  for (int vertexIndex = 0; vertexIndex < event.GetNumberOfPrimaryVertex();
       ++vertexIndex) {
    for (auto *primary = event.GetPrimaryVertex(vertexIndex)->GetPrimary();
         primary != nullptr; primary = primary->GetNext()) {
      if (primary->GetPDGcode() == -11)
        positron = primary;
      if (primary->GetPDGcode() == 11)
        electron = primary;
    }
  }

  if (positron && electron) {
    const auto positronMomentum = positron->GetMomentum();
    const auto electronMomentum = electron->GetMomentum();
    const double positronEnergy = std::sqrt(
        positronMomentum.mag2() + electron_mass_c2 * electron_mass_c2);
    const double electronEnergy = std::sqrt(
        electronMomentum.mag2() + electron_mass_c2 * electron_mass_c2);

    result.massMeV =
        std::sqrt(
            std::max(0.0, (positronEnergy + electronEnergy) *
                                  (positronEnergy + electronEnergy) -
                              (positronMomentum + electronMomentum).mag2())) /
        MeV;
    result.openingAngleDegrees =
        positronMomentum.angle(electronMomentum) / degree;
    result.positronEnergyMeV = positronEnergy / MeV;
    result.electronEnergyMeV = electronEnergy / MeV;

    for (int axis = 0; axis < 3; ++axis) {
      result.positronMomentumMeV[axis] = positronMomentum[axis] / MeV;
      result.electronMomentumMeV[axis] = electronMomentum[axis] / MeV;
    }
  }

  return result;
}
