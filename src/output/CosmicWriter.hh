#pragma once
#include "physics/CosmicObservables.hh"
#include "readout/BarHitReadout.hh"
#include "readout/DetectorDeposits.hh"
#include <array>
class G4Step;
class G4Event;
void WriteDetectorStep(int eventId, int layer, int arm, const G4Step &step);
void WriteArmDeposits(int eventId, const DetectorDeposits &deposits);
// adc[0] is the D-end PMT and adc[1] the U-end PMT; acquisitionTime is the _time shared by the two PMT rows.
void WriteRawHit(int eventId, int hitIndex, const BarHit &hit, double acquisitionTime,
                 const std::array<int, 2> &adc);
void WriteTruth(int eventId, int sourceId, const G4Event &event);
void WriteCosmicEvent(int eventId, const G4Event &event,
                      const CosmicObservables &observables);
