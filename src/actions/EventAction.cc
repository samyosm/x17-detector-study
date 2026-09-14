#include "EventAction.hh"
#include "source/PrimaryGeneratorAction.hh"

#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4PhysicalConstants.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>

namespace {
std::atomic<std::uint64_t> completedEvents{0};
}

EventAction::EventAction(const PrimaryGeneratorAction *source, int progressInterval)
    : source_(source), progressInterval_(progressInterval) {}

void EventAction::BeginOfEventAction(const G4Event *event) {
  eventId_ = event->GetEventID();
  barEnergy_.fill(0.0);
  pmtPhotons_.fill(0);
  firstPmtTimeNs_.fill(std::numeric_limits<double>::quiet_NaN());
}

void EventAction::AddBarEnergy(int bar, double energy) {
  if (bar >= 1 && bar <= static_cast<int>(barEnergy_.size())) {
    barEnergy_[bar - 1] += energy;
  }
}

void EventAction::AddPmtPhoton(int channel, double time, double energy) {
  if (channel < 1 || channel > static_cast<int>(pmtPhotons_.size())) return;
  const int index = channel - 1;
  const double timeNs = time / ns;
  if (pmtPhotons_[index] == 0 || timeNs < firstPmtTimeNs_[index]) {
    firstPmtTimeNs_[index] = timeNs;
  }
  ++pmtPhotons_[index];
  auto* analysis = G4AnalysisManager::Instance();
  analysis->FillNtupleIColumn(1, 0, eventId_);
  analysis->FillNtupleIColumn(1, 1, channel);
  analysis->FillNtupleDColumn(1, 2, timeNs);
  analysis->FillNtupleDColumn(1, 3, energy / eV);
  analysis->AddNtupleRow(1);
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
  for (int bar = 0; bar < 16; ++bar) {
    analysis->FillNtupleDColumn(12 + bar, barEnergy_[bar] / MeV);
  }
  for (int channel = 0; channel < 32; ++channel) {
    analysis->FillNtupleIColumn(28 + 2 * channel, pmtPhotons_[channel]);
    analysis->FillNtupleDColumn(29 + 2 * channel, firstPmtTimeNs_[channel]);
  }
  analysis->AddNtupleRow();
  const auto completed = completedEvents.fetch_add(1, std::memory_order_relaxed) + 1;
  if (completed % progressInterval_ == 0) {
    std::cout << completed << " events processed" << std::endl;
  }
}
