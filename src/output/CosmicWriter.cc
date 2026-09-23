#include "output/CosmicWriter.hh"
#include "G4Event.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "output/RootRow.hh"

namespace {
template <std::size_t IntegerCount, std::size_t RealCount>
void WriteRow(OutputTree tree, const int (&integers)[IntegerCount],
              const double (&reals)[RealCount]) {
  RootRow row(tree);
  for (const auto value : integers)
    row.Add(value);
  for (const auto value : reals)
    row.Add(value);
  row.Finish();
}
} // namespace

void WriteDetectorStep(int eventId, int layer, int arm, const G4Step &step) {
  const auto *track = step.GetTrack();
  const auto *start = step.GetPreStepPoint();
  const auto startPosition = start->GetPosition() / cm;
  const auto endPosition = step.GetPostStepPoint()->GetPosition() / cm;
  const int identifiers[] = {eventId,
                             layer,
                             arm + 1,
                             track->GetTrackID(),
                             track->GetParentID(),
                             track->GetDefinition()->GetPDGEncoding()};
  const double values[] = {step.GetTotalEnergyDeposit() / MeV,
                           start->GetGlobalTime() / ns,
                           startPosition.x(),
                           startPosition.y(),
                           startPosition.z(),
                           endPosition.x(),
                           endPosition.y(),
                           endPosition.z()};
  WriteRow(OutputTree::cosmicSteps, identifiers, values);
}

void WriteArmDeposits(int eventId, const DetectorDeposits &deposits) {
  for (int arm = 0; arm < detector::armCount; ++arm) {
    double totalEnergy = 0;
    double values[detector::layerCount * 4];
    for (int layer = 0; layer < detector::layerCount; ++layer) {
      const auto &deposit = deposits[layer][arm];
      totalEnergy += deposit.energy;
      values[4 * layer] = deposit.energy / MeV;
      const auto position = deposit.Centroid() / cm;
      for (int axis = 0; axis < 3; ++axis)
        values[4 * layer + 1 + axis] = position[axis];
    }
    if (totalEnergy == 0)
      continue;
    const int identifiers[] = {eventId, arm + 1};
    WriteRow(OutputTree::cosmicArms, identifiers, values);
  }
}

void WriteCosmicEvent(const G4Event &event,
                      const CosmicObservables &observables) {
  const auto *vertex = event.GetPrimaryVertex();
  const auto *muon = vertex->GetPrimary();
  const auto position = vertex->GetPosition() / cm;
  const auto direction = muon->GetMomentum().unit();
  const bool hasPair = observables.hitArms.size() == 2;
  const int identifiers[] = {event.GetEventID(),
                             muon->GetPDGcode(),
                             static_cast<int>(observables.hitArms.size()),
                             observables.hitMask,
                             hasPair ? observables.hitArms[0] + 1 : 0,
                             hasPair ? observables.hitArms[1] + 1 : 0};
  const double values[] = {muon->GetKineticEnergy() / MeV,
                           position.x(),
                           position.y(),
                           position.z(),
                           direction.x(),
                           direction.y(),
                           direction.z(),
                           -direction.y(),
                           observables.energySum / MeV,
                           observables.energyAsymmetry,
                           observables.openingAngle / degree,
                           observables.offsetOpeningAngle / degree};
  WriteRow(OutputTree::cosmicEvents, identifiers, values);
}
