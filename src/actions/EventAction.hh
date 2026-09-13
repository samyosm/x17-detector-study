#pragma once

#include "G4UserEventAction.hh"

#include <array>
class PrimaryGeneratorAction;

class EventAction final : public G4UserEventAction {
public:
    explicit EventAction(const PrimaryGeneratorAction* source);
    void BeginOfEventAction(const G4Event*) override;
    void EndOfEventAction(const G4Event* event) override;
    void AddBarEnergy(int bar, double energy);

private:
    std::array<double, 16> barEnergy_{};
    std::array<double, 32> pmtEnergy_{};
    int eventId_ = 0;
    int sourceMode_ = 0;
    double pairMassMeV_ = 0.0;
    double openingAngleDeg_ = 0.0;
    double positronEnergyMeV_ = 0.0;
    double electronEnergyMeV_ = 0.0;
    std::array<double, 3> positronMomentumMeV_{};
    std::array<double, 3> electronMomentumMeV_{};
    const PrimaryGeneratorAction* source_ = nullptr;
};
