#include "G4Event.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"
#include "output/CosmicWriter.hh"
#include "output/RootRow.hh"

void WriteRawHit(int eventId, const BarHit &hit) {
  RootRow row(OutputTree::rawHits);
  row.Add(eventId);
  row.Add(hit.bar);
  row.Add(hit.energy / MeV);
  row.Add(hit.position / cm);
  row.Add(hit.time / ns);
  row.Finish();
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
