#pragma once

#include "G4UserSteppingAction.hh"

class EventAction;

class SteppingAction final : public G4UserSteppingAction {
public:
    SteppingAction(EventAction* eventAction, double quantumEfficiency);
    void UserSteppingAction(const G4Step* step) override;

private:
    EventAction* eventAction_;
    double quantumEfficiency_;
};
