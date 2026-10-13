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
void RunProcess(const std::string &executable, std::vector<std::string> arguments,
                const std::string &failure) {
  arguments.insert(arguments.begin(), executable);
  std::vector<char *> pointers;
  for (auto &argument : arguments)
    pointers.push_back(argument.data());
  pointers.push_back(nullptr);
  pid_t process;
  if (posix_spawn(&process, executable.c_str(), nullptr, nullptr, pointers.data(), environ) != 0)
    throw std::runtime_error(failure);
  int status = 0;
  pid_t result;
  do {
    result = waitpid(process, &status, 0);
  } while (result == -1 && errno == EINTR);
  if (result == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
    throw std::runtime_error(failure);
}

void MergeRootFiles(const std::vector<std::string> &files,
                    const std::filesystem::path &output) {
  std::vector<std::string> arguments{"-n", "32", output.string()};
  arguments.insert(arguments.end(), files.begin(), files.end());
  RunProcess(ROOT_HADD_EXECUTABLE, arguments, "ROOT merge failed; checkpoints retained");
}
}

void EmbedRunInfo(const Configuration &config) {
  const auto output = config.Get<std::string>("runtime.output_file");
  RunProcess(ROOT_EXECUTABLE,
             {"-l", "-b", "-q", "-e",
              "auto file = TFile::Open(\"" + output + "\", \"UPDATE\"); "
              "std::ifstream input(\"" + output + ".simulation.toml\"); "
              "std::stringstream text; text << input.rdbuf(); "
              "TObjString info(text.str().c_str()); info.Write(\"run_info\"); file->Close();"},
             "Could not embed run information in " + output);
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
  if (KeepsRunRecords(config)) {
    for (const auto *extension : {".toml", ".gdml", ".run.txt", ".simulation.toml"})
      if (std::filesystem::exists(files.front() + extension))
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
