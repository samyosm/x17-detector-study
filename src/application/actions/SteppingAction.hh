#pragma once

#include "G4UserSteppingAction.hh"
#include "geometry/DetectorLayout.hh"
#include <vector>

class EventAction;
class G4LogicalVolume;

class SteppingAction final : public G4UserSteppingAction {
public:
    SteppingAction(EventAction* eventAction, double quantumEfficiency);
    void UserSteppingAction(const G4Step* step) override;

private:
    detector::Layer FindLayer(const G4LogicalVolume* volume) const;
    std::vector<detector::Layer> layers_;
    EventAction* eventAction_;
    double quantumEfficiency_;
};
