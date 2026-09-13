#include "EventAction.hh"
#include "source/PrimaryGeneratorAction.hh"

#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4PhysicalConstants.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"

#include <algorithm>
#include <cmath>
#include <limits>

EventAction::EventAction(const PrimaryGeneratorAction *source)
    : source_(source) {}

void EventAction::BeginOfEventAction(const G4Event *) { barEnergy_.fill(0.0); }

void EventAction::AddBarEnergy(int bar, double energy) {
  if (bar >= 1 && bar <= static_cast<int>(barEnergy_.size())) {
    barEnergy_[bar - 1] += energy;
  }
}

void EventAction::EndOfEventAction(const G4Event *event) {
  eventId_ = event->GetEventID();
  sourceMode_ = source_->GetModeId();

  const double missing = std::numeric_limits<double>::quiet_NaN();

  pairMassMeV_ = openingAngleDeg_ = missing;
  positronEnergyMeV_ = electronEnergyMeV_ = missing;
  positronMomentumMeV_.fill(missing);
  electronMomentumMeV_.fill(missing);

  const G4PrimaryParticle *positron = nullptr;
  const G4PrimaryParticle *electron = nullptr;

  for (int vertexIndex = 0; vertexIndex < event->GetNumberOfPrimaryVertex();
       ++vertexIndex) {
    for (auto *primary = event->GetPrimaryVertex(vertexIndex)->GetPrimary();
         primary != nullptr; primary = primary->GetNext()) {
      if (primary->GetPDGcode() == -11)
        positron = primary;
      if (primary->GetPDGcode() == 11)
        electron = primary;
    }
  }

  if (positron && electron) {
    const auto pPlus = positron->GetMomentum();
    const auto pMinus = electron->GetMomentum();
    const double ePlus =
        std::sqrt(pPlus.mag2() + electron_mass_c2 * electron_mass_c2);
    const double eMinus =
        std::sqrt(pMinus.mag2() + electron_mass_c2 * electron_mass_c2);

    pairMassMeV_ = std::sqrt(std::max(0.0, (ePlus + eMinus) * (ePlus + eMinus) -
                                               (pPlus + pMinus).mag2())) /
                   MeV;
    openingAngleDeg_ = pPlus.angle(pMinus) / degree;
    positronEnergyMeV_ = ePlus / MeV;
    electronEnergyMeV_ = eMinus / MeV;

    for (int axis = 0; axis < 3; ++axis) {
      positronMomentumMeV_[axis] = pPlus[axis] / MeV;
      electronMomentumMeV_[axis] = pMinus[axis] / MeV;
    }
  }

  for (int bar = 0; bar < static_cast<int>(barEnergy_.size()); ++bar) {
    // NOTE: Maybe we shoudl at some point in time simulate optical transport
    // (instead of justing registring MeV)
    pmtEnergy_[2 * bar] = barEnergy_[bar] / MeV;
    pmtEnergy_[2 * bar + 1] = barEnergy_[bar] / MeV;
  }

  auto* analysis = G4AnalysisManager::Instance();
  analysis->FillNtupleIColumn(0, eventId_);
  analysis->FillNtupleIColumn(1, sourceMode_);
  const double truth[] = {pairMassMeV_, openingAngleDeg_,
                          positronEnergyMeV_, electronEnergyMeV_};
  for (int i = 0; i < 4; ++i) analysis->FillNtupleDColumn(2 + i, truth[i]);
  for (int axis = 0; axis < 3; ++axis) {
    analysis->FillNtupleDColumn(6 + 2 * axis, positronMomentumMeV_[axis]);
    analysis->FillNtupleDColumn(7 + 2 * axis, electronMomentumMeV_[axis]);
  }
  for (int channel = 0; channel < 32; ++channel) {
    analysis->FillNtupleDColumn(12 + channel, pmtEnergy_[channel]);
  }
  analysis->AddNtupleRow();
}
