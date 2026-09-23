#pragma once
#include "physics/PairObservables.hh"
#include "readout/EventReadout.hh"

void DefineRootTrees();
void WriteEvent(const EventReadout &event, int sourceMode,
                const PairObservables &pair);
void WritePhotonHit(int eventId, int channel, double time, double energy);
