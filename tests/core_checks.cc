#include "G4AnalysisManager.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "configuration/Configuration.hh"
#include "geometry/CosmicSourcePlane.hh"
#include "geometry/DetectorLayout.hh"
#include "output/RootOutput.hh"
#include "physics/CosmicObservables.hh"
#include "physics/PairDecay.hh"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void Require(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

void CheckConfiguration() {
  const Configuration config(std::string(PROJECT_PATH) +
                             "/config/configuration.toml");
  Require(config.Get<bool>("source.ipc.enable"), "IPC preset changed");
  Require(config.Get<bool>("source.x17.enable"), "X17 preset changed");
  Require(!config.Get<bool>("missing.enable", false),
          "Optional flag does not default");
  Require(config.Vector("source.gun.direction")[2] == 1,
          "Vector lookup failed");
  Require(std::filesystem::exists(config.Path("geometry.file")),
          "Geometry path resolution failed");
  bool rejectedWrongType = false;
  try {
    config.Get<int>("runtime.mode", 0);
  } catch (const std::runtime_error &) {
    rejectedWrongType = true;
  }
  Require(rejectedWrongType, "Invalid value silently defaulted");
  const Configuration cosmic(std::string(PROJECT_PATH) +
                             "/config/cosmic_muons.toml");
  const CosmicSourcePlane plane(cosmic);
  for (const auto &corner : plane.Corners()) {
    Require(corner.y() == 60 * cm, "Source plane height changed");
    Require(std::abs(corner.x()) == 100 * cm &&
                std::abs(corner.z()) == 100 * cm,
            "Source plane dimensions changed");
  }
}

void CheckDetectorNumbering() {
  for (int arm = 0; arm < detector::armCount; ++arm) {
    const double angle = arm * twopi / detector::armCount;
    Require(detector::FindNearestArm({-std::sin(angle), -std::cos(angle), 0}) ==
                arm,
            "Clockwise arm numbering changed");
    Require(detector::FindEndSensor(arm, 1) == 2 * arm,
            "Negative-z sensor numbering changed");
    Require(detector::FindEndSensor(arm, -1) == 2 * arm + 1,
            "Positive-z sensor numbering changed");
  }
  Require(detector::FindSensitiveLayer("ScintillatorWrap") == detector::outside,
          "Wrapping is counted as scintillator");
  Require(detector::FindSensitiveLayer("DeltaEScintillator_03") ==
              detector::deltaE,
          "DeltaE layer recognition failed");
}

void CheckCosmicObservables() {
  DetectorDeposits deposits{};
  const EnergyThresholds thresholds{0.1 * MeV, 0.05 * MeV, 0.001 * MeV};
  auto result = CalculateCosmicObservables(deposits, thresholds);
  Require(result.hitArms.empty() && std::isnan(result.energySum),
          "Miss must have undefined pair energy");
  for (int arm : {0, 8}) {
    deposits[detector::scintillator][arm].Add(10 * MeV, {0, 0, 0});
    deposits[detector::deltaE][arm].Add(0.1 * MeV, {0, 0, 0});
    deposits[detector::chamber][arm].Add(0.01 * MeV,
                                         {0, arm == 0 ? -6 * cm : 6 * cm, 0});
  }
  result = CalculateCosmicObservables(deposits, thresholds);
  Require(result.hitArms.size() == 2 && result.hitMask == 257,
          "Coincidence selection changed");
  Require(std::abs(result.energySum / MeV - 20.2) < 1e-12,
          "Pair sum must exclude chamber energy");
  Require(result.energyAsymmetry == 0,
          "Equal deposits must have zero asymmetry");
  Require(std::abs(result.openingAngle - pi) < 1e-12,
          "Opposite hits must subtend 180 degrees");
  Require(result.offsetOpeningAngle < result.openingAngle,
          "Vertex offset is ignored");
  deposits[detector::deltaE][8].energy = thresholds[detector::deltaE];
  result = CalculateCosmicObservables(deposits, thresholds);
  Require(result.hitArms.size() == 1 && std::isnan(result.energySum),
          "Threshold comparison must be strict");
  EnergyDeposit deposit;
  deposit.Add(1, {0, 0, 0});
  deposit.Add(3, {4, 0, 0});
  Require(deposit.Centroid().x() == 3,
          "Deposit centroid is not energy weighted");
}

void CheckTwoBodyMomentum() {
  const double momentum = CalculateTwoBodyMomentum(10, 3, 2);
  Require(std::abs(std::hypot(3, momentum) + std::hypot(2, momentum) - 10) <
              1e-12,
          "Two-body energies do not conserve parent energy");
  Require(CalculateTwoBodyMomentum(5, 3, 2) == 0,
          "Threshold decay must have zero momentum");
}

void CheckRootSchema() {
  DefineRootTrees();
  auto *output = G4AnalysisManager::Instance();
  const char *names[] = {"events", "photon_hits", "cosmic_arms",
                         "cosmic_events", "cosmic_steps"};
  const std::size_t columns[] = {92, 4, 14, 18, 14};
  Require(output->GetNofNtuples() == 5, "Output tree count changed");
  for (int tree = 0; tree < 5; ++tree) {
    const auto *booking = output->GetNtuple(tree);
    Require(booking && booking->name() == names[tree],
            "Output tree order changed");
    Require(booking->columns().size() == columns[tree],
            "Output column count changed");
  }
  const auto &eventColumns = output->GetNtuple(0)->columns();
  Require(eventColumns[12].name() == "scint_01_edep_MeV",
          "Scintillator column order changed");
  Require(eventColumns[28].name() == "pmt_01_D_photons",
          "Sensor column order changed");
  Require(eventColumns[91].name() == "pmt_16_U_first_time_ns",
          "Last sensor column changed");
  Require(output->GetNtuple(3)->columns()[17].name() ==
              "opening_angle_z25mm_deg",
          "Offset-angle column changed");
}
} // namespace

int main() {
  try {
    CheckConfiguration();
    CheckDetectorNumbering();
    CheckCosmicObservables();
    CheckTwoBodyMomentum();
    CheckRootSchema();
    std::cout
        << "Core checks passed without generating or transporting events.\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
