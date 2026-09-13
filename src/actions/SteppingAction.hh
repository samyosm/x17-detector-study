#pragma once

#include "G4UserSteppingAction.hh"

class EventAction;

class SteppingAction final : public G4UserSteppingAction {
public:
    explicit SteppingAction(EventAction* eventAction);
    void UserSteppingAction(const G4Step* step) override;

private:
    EventAction* eventAction_;
};
