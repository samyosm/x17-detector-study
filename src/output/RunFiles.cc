#include "output/RunFiles.hh"
#include "G4Version.hh"
#include "Randomize.hh"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

int CheckpointEventCount(const Configuration &config) {
  const auto count = config.Get<int>("runtime.checkpoint_events", 1000000);
  if (count <= 0)
    throw std::runtime_error("runtime.checkpoint_events must be positive");
  return count;
}

bool UsesCheckpoints(const Configuration &config) {
  return config.Get<std::string>("runtime.mode") == "batch" &&
         config.Get<int>("runtime.events") > CheckpointEventCount(config);
}

std::filesystem::path RunOutputFile(const Configuration &config, int runId) {
  const auto output = config.Get<std::string>("runtime.output_file");
  if (!UsesCheckpoints(config))
    return output;
  std::ostringstream name;
  name << "part-" << std::setw(6) << std::setfill('0') << runId + 1 << ".root";
  return std::filesystem::path(output + ".parts") / name.str();
}

std::filesystem::path PendingRunOutputFile(const Configuration &config, int runId) {
  auto path = RunOutputFile(config, runId);
  if (UsesCheckpoints(config))
    path.replace_extension(".partial.root");
  return path;
}

void PrepareBatchOutput(const Configuration &config) {
  if (!UsesCheckpoints(config))
    return;
  const auto output = config.Get<std::string>("runtime.output_file");
  const auto directory = output + ".parts";
  if (std::filesystem::exists(output) || std::filesystem::exists(directory))
    throw std::runtime_error("Output already exists: " + output + " or " + directory);
  std::filesystem::create_directories(directory);
}

void PrepareRunFiles(const Configuration &config, int runId, double cosmicRateHz) {
  const auto outputFile = RunOutputFile(config, runId).string();
  const auto parent = std::filesystem::path(outputFile).parent_path();
  if (!parent.empty())
    std::filesystem::create_directories(parent);
  if (std::filesystem::exists(outputFile) ||
      std::filesystem::exists(PendingRunOutputFile(config, runId)))
    throw std::runtime_error("Output already exists: " + outputFile);
  if (!config.Get<bool>("source.cosmic_muons.enable", false))
    return;
  std::filesystem::copy_file(config.File(), outputFile + ".toml",
                             std::filesystem::copy_options::overwrite_existing);
  std::filesystem::copy_file(config.Path("geometry.file"), outputFile + ".gdml",
                             std::filesystem::copy_options::overwrite_existing);
  const int offset = UsesCheckpoints(config) ? runId * CheckpointEventCount(config) : 0;
  const int requested = UsesCheckpoints(config)
      ? std::min(CheckpointEventCount(config), config.Get<int>("runtime.events") - offset)
      : config.Get<int>("runtime.events");
  std::ofstream manifest(outputFile + ".run.txt");
  manifest << G4Version
           << "\nPhysics: FTFP_BERT + G4EmStandardPhysics_option3\n"
           << "Optical physics: " << config.Get<bool>("optics.enable", false)
           << "\nSeed: " << config.Get<long>("runtime.random_seed")
           << "\nRandom engine: " << G4Random::getTheEngine()->name()
           << "\nRequested events: " << config.Get<int>("runtime.events")
           << "\nCheckpoint events: " << requested
           << "\nFirst event ID: " << offset
           << "\nThreads: " << config.Get<int>("runtime.threads") << '\n';
  manifest << "Source model: Guan et al. 2015, arXiv:1509.06176, equations 2-3\n"
           << "Source energy: kinetic bounds; flux evaluated at total energy\n"
           << "Source angular measure: horizontal projected area, uniform azimuth\n"
           << "Source rate Hz: " << std::setprecision(17)
           << cosmicRateHz << '\n';
  manifest.close();
  if (!manifest)
    throw std::runtime_error("Could not write run manifest: " + outputFile);
}

void CompleteRunFile(const Configuration &config, int runId) {
  if (!UsesCheckpoints(config))
    return;
  const auto output = RunOutputFile(config, runId);
  if (std::filesystem::exists(output))
    throw std::runtime_error("Output already exists: " + output.string());
  std::filesystem::rename(PendingRunOutputFile(config, runId), output);
}
