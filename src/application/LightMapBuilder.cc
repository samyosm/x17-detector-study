#include "application/LightMapBuilder.hh"
#include "G4AffineTransform.hh"
#include "G4Event.hh"
#include "G4Navigator.hh"
#include "G4OpticalPhoton.hh"
#include "G4ParticleGun.hh"
#include "G4RunManagerFactory.hh"
#include "G4VModularPhysicsList.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4TransportationManager.hh"
#include "G4UserEventAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4VUserActionInitialization.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "Randomize.hh"
#include "geometry/DetectorConstruction.hh"
#include "physics/OpticalMaterials.hh"
#include "physics/PhysicsList.hh"
#include "readout/LightMap.hh"
#include "readout/OpticalSensor.hh"
#include <algorithm>
#include <array>
#include <iostream>
#include <mutex>
#include <stdexcept>

namespace {
struct Grid {
  int bandsY, bandsDepth, bandsWidth;
  int photonsPerEvent;
  int eventsPerCell;
};

struct Tally {
  std::mutex lock;
  std::vector<std::array<double, 2>> detected;
  std::vector<double> generated;
};

// Bar copy 0 sits at (0, -20.75 cm, 0) in the world.
const G4ThreeVector referenceBarCenter(0, -207.5 * mm, 0);

class Primaries final : public G4VUserPrimaryGeneratorAction {
public:
  Primaries(const Grid &grid, double energy)
      : grid_(grid), gun_(1), energy_(energy), shape_(FindBarShape()) {
    gun_.SetParticleDefinition(G4OpticalPhoton::Definition());
    gun_.SetParticleEnergy(energy_);
  }

  void GeneratePrimaries(G4Event *event) override {
    if (!navigator_) {
      navigator_ = std::make_unique<G4Navigator>();
      navigator_->SetWorldVolume(
          G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking()->GetWorldVolume());
      G4TouchableHistory history;
      auto *found = navigator_->LocateGlobalPointAndSetup(referenceBarCenter, nullptr, false);
      if (!found || found->GetCopyNo() != 0 || found->GetLogicalVolume()->GetName() != "Scintillator")
        throw std::runtime_error("Could not find scintillator bar 0 for the light map");
      const auto touchable = navigator_->CreateTouchableHistory();
      toWorld_ = touchable->GetHistory()->GetTopTransform().Inverse();
      delete touchable;
    }
    const int cell = event->GetEventID() / grid_.eventsPerCell;
    const int ix = cell % grid_.bandsWidth;
    const int iz = (cell / grid_.bandsWidth) % grid_.bandsDepth;
    const int iy = cell / (grid_.bandsWidth * grid_.bandsDepth);
    for (int photon = 0; photon < grid_.photonsPerEvent; ++photon) {
      const auto local = LightMap::LocalPoint(shape_, (iy + G4UniformRand()) / grid_.bandsY,
                                              (iz + G4UniformRand()) / grid_.bandsDepth,
                                              (ix + G4UniformRand()) / grid_.bandsWidth);
      const double cosine = 2 * G4UniformRand() - 1;
      const double sine = std::sqrt(1 - cosine * cosine);
      const double azimuth = CLHEP::twopi * G4UniformRand();
      const G4ThreeVector direction(sine * std::cos(azimuth), sine * std::sin(azimuth), cosine);
      const G4ThreeVector perpendicular = direction.orthogonal().unit();
      const double angle = CLHEP::twopi * G4UniformRand();
      const G4ThreeVector polarization =
          perpendicular * std::cos(angle) + direction.cross(perpendicular) * std::sin(angle);
      gun_.SetParticlePosition(toWorld_.TransformPoint(local));
      gun_.SetParticleMomentumDirection(direction);
      gun_.SetParticlePolarization(polarization);
      gun_.GeneratePrimaryVertex(event);
    }
  }

private:
  Grid grid_;
  G4ParticleGun gun_;
  double energy_;
  BarShape shape_;
  std::unique_ptr<G4Navigator> navigator_;
  G4AffineTransform toWorld_;
};

class Counting final : public G4UserEventAction {
public:
  Counting(const Grid &grid, Tally &tally) : grid_(grid), tally_(tally) {}
  void BeginOfEventAction(const G4Event *) override { detected_ = {0, 0}; }
  void Detect(int end) { ++detected_[end]; }
  void EndOfEventAction(const G4Event *event) override {
    const int cell = event->GetEventID() / grid_.eventsPerCell;
    std::lock_guard<std::mutex> guard(tally_.lock);
    tally_.generated[cell] += grid_.photonsPerEvent;
    for (int end = 0; end < 2; ++end)
      tally_.detected[cell][end] += detected_[end];
  }

private:
  Grid grid_;
  Tally &tally_;
  std::array<long, 2> detected_{0, 0};
};

class Stepping final : public G4UserSteppingAction {
public:
  explicit Stepping(Counting &counting) : counting_(counting) {}
  void UserSteppingAction(const G4Step *step) override {
    const int sensor = FindCrossedEndSensor(*step);
    if (sensor < 0)
      return;
    if (sensor < 2)
      counting_.Detect(sensor);
    step->GetTrack()->SetTrackStatus(fStopAndKill);
  }

private:
  Counting &counting_;
};

class Actions final : public G4VUserActionInitialization {
public:
  Actions(const Grid &grid, double energy, Tally &tally) : grid_(grid), energy_(energy), tally_(tally) {}
  void Build() const override {
    SetUserAction(new Primaries(grid_, energy_));
    auto *counting = new Counting(grid_, tally_);
    SetUserAction(counting);
    SetUserAction(new Stepping(*counting));
  }

private:
  Grid grid_;
  double energy_;
  Tally &tally_;
};
} // namespace

void BuildLightMap(Configuration config) {
  config.Set("optics.mode", std::string("tracking"));
  config.Set("optics.fold_efficiency_into_yield", false);
  config.Set("source.lab.enable", false);
  config.Set("source.cosmic_muons.enable", false);
  Grid grid{config.Get<int>("optics.light_map.bands_y"), config.Get<int>("optics.light_map.bands_depth"),
            config.Get<int>("optics.light_map.bands_width"), 1000, 0};
  const long photonsPerCell = config.Get<long>("optics.light_map.photons_per_cell");
  grid.eventsPerCell = static_cast<int>(std::max<long>(1, photonsPerCell / grid.photonsPerEvent));
  const double energy = 0.5 * (config.Get<double>("optics.scintillator.emission_min_ev") +
                               config.Get<double>("optics.scintillator.emission_max_ev")) * eV;
  LightMap map(grid.bandsY, grid.bandsDepth, grid.bandsWidth);
  Tally tally;
  tally.detected.assign(map.CellCount(), {0.0, 0.0});
  tally.generated.assign(map.CellCount(), 0.0);

  G4Random::setTheSeed(config.Get<long>("runtime.random_seed"));
  auto manager = std::unique_ptr<G4RunManager>(G4RunManagerFactory::CreateRunManager(G4RunManagerType::MT));
  manager->SetNumberOfThreads(config.Get<int>("runtime.threads"));
  manager->SetUserInitialization(new DetectorConstruction(config.Path("geometry.file").string(), config));
  manager->SetUserInitialization(CreatePhysicsList(config));
  manager->SetUserInitialization(new Actions(grid, energy, tally));
  manager->Initialize();
  const long events = static_cast<long>(map.CellCount()) * grid.eventsPerCell;
  std::cout << "Tracking " << events * grid.photonsPerEvent << " optical photons over " << map.CellCount()
            << " cells" << std::endl;
  manager->BeamOn(static_cast<int>(events));

  double lowest = 1, highest = 0;
  for (std::size_t cell = 0; cell < map.CellCount(); ++cell) {
    const double d = tally.detected[cell][0] / tally.generated[cell];
    const double u = tally.detected[cell][1] / tally.generated[cell];
    map.Set(cell, d, u);
    lowest = std::min({lowest, d, u});
    highest = std::max({highest, d, u});
  }
  auto metadata = LightMap::Describe(config);
  const auto shape = FindBarShape();
  metadata["photons_per_cell"] = std::to_string(grid.eventsPerCell * grid.photonsPerEvent);
  metadata["bar_half_length_cm"] = std::to_string(shape.halfLength / cm);
  metadata["bar_half_depth_cm"] = std::to_string(shape.halfDepth / cm);
  metadata["bar_half_width_back_cm"] = std::to_string(shape.halfWidthAtBack / cm);
  metadata["bar_half_width_front_cm"] = std::to_string(shape.halfWidthAtFront / cm);
  metadata["seed"] = std::to_string(config.Get<long>("runtime.random_seed"));
  const auto file = LightMap::File(config);
  std::filesystem::create_directories(file.parent_path());
  map.Save(file, metadata);
  std::cout << "Wrote " << file << " (collection probability per cell from " << lowest << " to " << highest << ")"
            << std::endl;
}
