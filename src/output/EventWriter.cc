#include "G4SystemOfUnits.hh"
#include "output/RootOutput.hh"
#include "output/RootRow.hh"

void WriteEvent(const EventReadout &event, int sourceMode,
                const PairObservables &pair) {
  RootRow row(OutputTree::events);
  row.Add(event.eventId);
  row.Add(sourceMode);
  row.Add(pair.massMeV);
  row.Add(pair.openingAngleDegrees);
  row.Add(pair.positronEnergyMeV);
  row.Add(pair.electronEnergyMeV);
  for (int axis = 0; axis < 3; ++axis) {
    row.Add(pair.positronMomentumMeV[axis]);
    row.Add(pair.electronMomentumMeV[axis]);
  }
  for (const auto energy : event.depositedEnergy)
    row.Add(energy / MeV);
  for (int sensor = 0; sensor < detector::sensorCount; ++sensor) {
    row.Add(event.photonCount[sensor]);
    row.Add(event.firstPhotonTimeNs[sensor]);
  }
  row.Finish();
}

void WritePhotonHit(int eventId, int channel, double time, double energy) {
  RootRow row(OutputTree::photonHits);
  row.Add(eventId);
  row.Add(channel);
  row.Add(time / ns);
  row.Add(energy / eV);
  row.Finish();
}
