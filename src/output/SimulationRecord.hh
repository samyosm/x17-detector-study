#pragma once
#include "configuration/Configuration.hh"
#include <filesystem>

void WriteSimulationRecord(const Configuration &config,
                           const std::filesystem::path &file,
                           double cosmicRateHz);
