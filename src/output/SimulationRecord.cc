#include "output/SimulationRecord.hh"
#include "physics/LabSourceDefinition.hh"
#include <fstream>
#include <iomanip>
#include <stdexcept>

void WriteSimulationRecord(const Configuration &config,
                           const std::filesystem::path &file,
                           double cosmicRateHz) {
  const bool lab = config.Get<bool>("source.lab.enable", false);
  const double decays = config.Get<int>("runtime.events");
  std::ofstream record(file);
  record << std::setprecision(17) << "[simulation]\n"
         << "id = \"" << config.Get<std::string>("simulation.id", file.stem().stem().string()) << "\"\n"
         << "events = " << decays << "\n"
         << "seed = " << config.Get<long>("runtime.random_seed") << "\n"
         << "geometry = \"" << config.Path("geometry.file").filename().string() << "\"\n"
         << "physics = \"FTFP_BERT + G4EmStandardPhysics_option3"
         << (lab ? " + G4RadioactiveDecayPhysics" : "") << "\"\n"
         << "hit_window_ns = " << config.Get<double>("readout.raw_hits.window_ns") << "\n";
  if (lab) {
    auto source = ReadLabSource(config);
    ResolveActivity(source);
    record << "\n[source]\nkind = \"lab\"\n"
           << "lab_id = " << source.labId << "\n"
           << "isotope = \"" << source.name << "\"\n"
           << "reference_activity_bq = " << source.referenceActivityBq << "\n"
           << "reference_date = \"" << source.referenceDate << "\"\n"
           << "activity_date = \"" << source.activityDate << "\"\n"
           << "half_life_s = " << source.halfLifeSeconds << "\n"
           << "activity_bq = " << source.activityBq << "\n"
           << "equivalent_real_time_s = " << decays / source.activityBq << "\n";
    record << "\n[placement]\nmin_cm = [" << source.placementMinimum.x() / 10 << ", "
           << source.placementMinimum.y() / 10 << ", " << source.placementMinimum.z() / 10
           << "]\nmax_cm = [" << source.placementMaximum.x() / 10 << ", "
           << source.placementMaximum.y() / 10 << ", " << source.placementMaximum.z() / 10 << "]\n";
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
