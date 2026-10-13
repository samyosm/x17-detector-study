#include "readout/Vf48Channels.hh"
#include "geometry/DetectorLayout.hh"
#include "output/SimulationRecord.hh"
#include "G4SystemOfUnits.hh"
#include "G4Version.hh"
#include "physics/LabSourceDefinition.hh"
#include "output/RunFiles.hh"
#include "physics/PhysicsList.hh"
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace {
std::string FileFingerprint(const std::filesystem::path &file) {
  std::ifstream input(file, std::ios::binary);
  std::uint64_t hash = 14695981039346656037ull;
  for (std::istreambuf_iterator<char> it(input), end; it != end; ++it)
    hash = (hash ^ static_cast<unsigned char>(*it)) * 1099511628211ull;
  std::ostringstream text;
  text << std::hex << std::setw(16) << std::setfill('0') << hash;
  return text.str();
}

void WriteVector(std::ostream &record, const std::string &name, const G4ThreeVector &v) {
  record << name << " = [" << v.x() << ", " << v.y() << ", " << v.z() << "]\n";
}
} // namespace

void WriteSimulationRecord(const Configuration &config,
                           const std::filesystem::path &file,
                           double cosmicRateHz) {
  const bool lab = config.Get<bool>("source.lab.enable", false);
  const double decays = config.Get<int>("runtime.events");
  std::ofstream record(file);
  record << std::setprecision(17) << "[simulation]\n"
         << "id = \"" << config.Get<std::string>("simulation.id", file.stem().stem().string()) << "\"\n"
         << "primaries = " << decays << "\n"
         << "seed = " << config.Get<long>("runtime.random_seed") << "\n"
         << "geometry = \"" << config.Path("geometry.file").filename().string() << "\"\n"
         << "geometry_version = \"" << config.Get<std::string>("geometry.version", "unspecified") << "\"\n"
         << "geometry_fingerprint_fnv1a64 = \"" << FileFingerprint(config.Path("geometry.file")) << "\"\n"
         << "geant4 = \"" << G4Version << "\"\n"
         << "physics = \"" << DescribePhysics(config) << "\"\n"
         << "hit_window_ns = " << config.Get<double>("readout.raw_hits.window_ns") << "\n"
         << "hit_definition = \"One scintillator hit groups the energy deposits of one bar within an event: "
            "deposits are sorted by time and a hit starts at its first deposit and collects every deposit "
            "up to hit_window_ns after it; the next deposit later than that starts a new hit. edep_MeV is the "
            "summed deposit, time_ns the energy-weighted mean deposit time since the decay of the primary "
            "ion, and z_cm (hit_truth) the energy-weighted mean global z of the deposits.\"\n";
  if (config.Get<bool>("readout.raw_hits.enable", false)) {
    record << "\n[vf48]\n"
           << "tree = \"vf48\": _time (ns), _field, ADC; two rows per hit (D and U PMT), both with the same _time\n"
           << "adc_definition = \"detected photoelectrons x adc_per_photoelectron, rounded and capped at 32767; "
              "no pedestal, noise or trigger rule\"\n"
           << "adc_per_photoelectron = " << config.Get<double>("readout.vf48.adc_per_photoelectron") << "\n"
           << "time_definition = \"_time = event_id x event_period_ns + the hit time since the decay rounded to "
              "time_resolution_ns, moved up by whole steps when two hits of an event land on the same value, so that every "
              "hit has its own _time; a synthetic acquisition clock, not the physical interaction time\"\n"
           << "time_resolution_ns = " << config.Get<double>("readout.vf48.time_resolution_ns") << "\n"
           << "event_period_ns = " << config.Get<double>("readout.vf48.event_period_ns") << "\n"
           << "scintillator_index = \"bar - 1\"\n"
           << "photon_to_hit = \"a detected photon belongs to the latest hit of its bar that started up to 1 ns before it\"\n";
    for (int scintillator = 0; scintillator < detector::armCount; ++scintillator)
      for (bool uEnd : {false, true})
        record << "channel_" << vf48::ChannelLabel(scintillator, uEnd) << " = \""
               << vf48::FieldName(scintillator, uEnd) << "\"\n";
    if (config.Get<bool>("optics.enable", false))
      record << "\n[optics]\n"
             << "yield_per_mev = " << config.Get<double>("optics.scintillator.yield_per_mev") << "\n"
             << "absorption_length_cm = " << config.Get<double>("optics.scintillator.absorption_length_cm") << "\n"
             << "refractive_index = " << config.Get<double>("optics.scintillator.refractive_index") << "\n"
             << "wrap = \"diffuse (Lambertian) reflector on every bar face; PLACEHOLDER reflectivity, not measured\"\n"
             << "wrap_reflectivity = " << config.Get<double>("optics.wrap.reflectivity") << "\n"
             << "decay_ns = " << config.Get<double>("optics.scintillator.decay_ns") << "\n"
             << "sensor_quantum_efficiency = " << config.Get<double>("optics.sensor.quantum_efficiency") << "\n";
  }
  if (lab) {
    auto source = ReadLabSource(config);
    ResolveActivity(source);
    record << "\n[source]\nkind = \"lab\"\n"
           << "lab_id = " << source.labId << "\n"
           << "isotope = \"" << source.name << "\"\n"
           << "encapsulated = " << (source.encapsulated ? "true" : "false") << "\n"
           << "reference_activity_bq = " << source.referenceActivityBq << "\n"
           << "reference_date = \"" << source.referenceDate << "\"\n"
           << "activity_date = \"" << source.activityDate << "\"\n"
           << "half_life_s = " << source.halfLifeSeconds << "\n"
           << "activity_bq = " << source.activityBq << "\n"
           << "equivalent_real_time_s = " << decays / source.activityBq << "\n";
    const auto &placement = source.placement;
    record << "\n[placement]\nx_cm = " << placement.x / cm << "\ny_cm = " << placement.y / cm << "\n"
           << "z_mode = \"" << (placement.uniformZ ? "uniform" : "fixed") << "\"\n";
    if (placement.uniformZ) {
      record << "z_min_cm = " << placement.zMinimum / cm << "\nz_max_cm = " << placement.zMaximum / cm << "\n";
      if (source.encapsulated)
        record << "z_positions = " << 1 + (config.Get<int>("runtime.events") - 1) / CheckpointEventCount(config)
               << "\nz_positions_note = \"the capsule is moved once per checkpoint; positions are stratified "
                  "uniform over the range\"\n";
      else
        record << "z_positions_note = \"vertex z drawn uniformly per decay\"\n";
    } else {
      record << "z_cm = " << placement.zFixed / cm << "\n";
    }
    WriteVector(record, "orientation", placement.orientation);
    if (source.encapsulated) {
      const auto &capsule = source.capsule;
      record << "\n[capsule]\ndescription = \"" << capsule.description << "\"\n"
             << "body_material = \"" << capsule.bodyMaterial << "\"\n"
             << "cavity_material = \"" << capsule.cavityMaterial << "\"\n"
             << "window_material = \"" << capsule.windowMaterial << "\"\n"
             << "active_material = \"" << capsule.activeMaterial << "\"\n"
             << "body_radius_mm = " << capsule.bodyRadius / mm << "\n"
             << "body_length_mm = " << capsule.bodyLength / mm << "\n"
             << "cavity_radius_mm = " << capsule.cavityRadius / mm << "\n"
             << "cavity_depth_mm = " << capsule.cavityDepth / mm << "\n"
             << "window_thickness_mm = " << capsule.windowThickness / mm << "\n"
             << "active_radius_mm = " << capsule.activeRadius / mm << "\n"
             << "active_thickness_mm = " << capsule.activeThickness / mm << "\n";
    }
  } else {
    record << "\n[source]\nkind = \"cosmic_muons\"\n"
           << "model = \"" << config.Get<std::string>("source.cosmic_muons.model", "guan") << "\"\n";
    if (cosmicRateHz > 0)
      record << "rate_hz = " << cosmicRateHz << "\n"
             << "equivalent_real_time_s = " << decays / cosmicRateHz << "\n";
  }
  record.close();
  if (!record)
    throw std::runtime_error("Could not write simulation record: " + file.string());
}
