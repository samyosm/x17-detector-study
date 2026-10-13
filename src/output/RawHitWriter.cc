#include "G4Event.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"
#include "output/CosmicWriter.hh"
#include "output/RootRow.hh"
#include "readout/Vf48Channels.hh"
#include <array>

void WriteRawHit(int eventId, int hitIndex, const BarHit &hit, double acquisitionTime,
                 const std::array<int, 2> &adc) {
  const double time = acquisitionTime / ns;
  const int scintillator = hit.bar - 1;
  for (int end = 0; end < 2; ++end) {
    RootRow row(OutputTree::vf48);
    row.Add(time);
    row.Add(vf48::FieldName(scintillator, end == 1));
    row.Add(adc[end]);
    row.Finish();
  }
  RootRow truth(OutputTree::hitTruth);
  truth.Add(eventId);
  truth.Add(hitIndex);
  truth.Add(hit.bar);
  truth.Add(hit.energy / MeV);
  truth.Add(hit.time / ns);
  truth.Add(hit.position / cm);
  truth.Add(time);
  truth.Finish();
}

void WriteTruth(int eventId, int sourceId, const G4Event &event) {
  const auto *vertex = event.GetPrimaryVertex();
  const auto position = vertex->GetPosition() / cm;
  RootRow row(OutputTree::truth);
  row.Add(eventId);
  row.Add(sourceId);
  row.Add(position.x());
  row.Add(position.y());
  row.Add(position.z());
  row.Add(vertex->GetPrimary()->GetKineticEnergy() / MeV);
  row.Finish();
}
