#pragma once
#include "physics/CosmicObservables.hh"
#include "readout/DetectorDeposits.hh"
class G4Step;
class G4Event;
void WriteDetectorStep(int eventId, int layer, int arm, const G4Step &step);
void WriteArmDeposits(int eventId, const DetectorDeposits &deposits);
void WriteCosmicEvent(const G4Event &event,
                      const CosmicObservables &observables);
