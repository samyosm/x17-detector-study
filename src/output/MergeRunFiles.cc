#include "output/RunFiles.hh"
#include <cerrno>
#include <fstream>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <vector>

extern char **environ;

namespace {
void MergeRootFiles(const std::vector<std::string> &files,
                    const std::filesystem::path &output) {
  std::vector<std::string> arguments{ROOT_HADD_EXECUTABLE, "-n", "32", output.string()};
  arguments.insert(arguments.end(), files.begin(), files.end());
  std::vector<char *> pointers;
  for (auto &argument : arguments)
    pointers.push_back(argument.data());
  pointers.push_back(nullptr);
  pid_t process;
  if (posix_spawn(&process, ROOT_HADD_EXECUTABLE, nullptr, nullptr,
                  pointers.data(), environ) != 0)
    throw std::runtime_error("Could not start ROOT merger; checkpoints retained");
  int status = 0;
  pid_t result;
  do {
    result = waitpid(process, &status, 0);
  } while (result == -1 && errno == EINTR);
  if (result == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
    throw std::runtime_error("ROOT merge failed; checkpoints retained");
}
}

void MergeCompletedRun(const Configuration &config) {
  if (!UsesCheckpoints(config))
    return;
  const std::filesystem::path output = config.Get<std::string>("runtime.output_file");
  const auto directory = output.string() + ".parts";
  const auto temporary = std::filesystem::path(directory) / "merged.partial.root";
  if (std::filesystem::exists(output) || std::filesystem::exists(temporary))
    throw std::runtime_error("Merge output already exists; checkpoints retained");
  const int events = config.Get<int>("runtime.events");
  const int count = 1 + (events - 1) / CheckpointEventCount(config);
  std::vector<std::string> files;
  for (int run = 0; run < count; ++run)
    files.push_back(RunOutputFile(config, run).string());
  MergeRootFiles(files, temporary);
  if (config.Get<bool>("source.cosmic_muons.enable", false)) {
    for (const auto *extension : {".toml", ".gdml", ".run.txt"})
      std::filesystem::copy_file(files.front() + extension, output.string() + extension,
                                std::filesystem::copy_options::overwrite_existing);
    std::ofstream manifest(output.string() + ".run.txt", std::ios::app);
    manifest << "Merged checkpoints: " << count << "\nCompleted events: " << events << '\n';
    manifest.close();
    if (!manifest)
      throw std::runtime_error("Could not write merged run manifest; checkpoints retained");
  }
  if (std::filesystem::exists(output))
    throw std::runtime_error("Final output already exists; checkpoints retained");
  std::filesystem::rename(temporary, output);
  std::filesystem::remove_all(directory);
}
