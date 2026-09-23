#include "output/RunFiles.hh"
#include "G4Version.hh"
#include "Randomize.hh"
#include <filesystem>
#include <fstream>
#include <stdexcept>

void PrepareRunFiles(const Configuration &config) {
  const auto outputFile = config.Get<std::string>("runtime.output_file");
  const auto parent = std::filesystem::path(outputFile).parent_path();
  if (!parent.empty())
    std::filesystem::create_directories(parent);
  if (!config.Get<bool>("source.cosmic_muons.enable", false))
    return;
  if (std::filesystem::exists(outputFile))
    throw std::runtime_error("Output already exists: " + outputFile);
  std::filesystem::copy_file(config.File(), outputFile + ".toml",
                             std::filesystem::copy_options::overwrite_existing);
  std::filesystem::copy_file(config.Path("geometry.file"), outputFile + ".gdml",
                             std::filesystem::copy_options::overwrite_existing);
  std::ofstream manifest(outputFile + ".run.txt");
  manifest << G4Version
           << "\nPhysics: FTFP_BERT + G4EmStandardPhysics_option3\n"
           << "Optical physics: " << config.Get<bool>("optics.enable", false)
           << "\nSeed: " << config.Get<long>("runtime.random_seed")
           << "\nRandom engine: " << G4Random::getTheEngine()->name()
           << "\nRequested events: " << config.Get<int>("runtime.events")
           << "\nThreads: " << config.Get<int>("runtime.threads") << '\n';
  if (!manifest)
    throw std::runtime_error("Could not write run manifest: " + outputFile);
}
