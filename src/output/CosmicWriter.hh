#pragma once
#include "physics/CosmicObservables.hh"
#include "readout/BarHitReadout.hh"
#include "readout/DetectorDeposits.hh"
class G4Step;
class G4Event;
void WriteDetectorStep(int eventId, int layer, int arm, const G4Step &step);
void WriteArmDeposits(int eventId, const DetectorDeposits &deposits);
void WriteRawHit(int eventId, const BarHit &hit);
void WriteTruth(int eventId, int sourceId, const G4Event &event);
void WriteCosmicEvent(int eventId, const G4Event &event,
                      const CosmicObservables &observables);
