#pragma once

#include <array>
#include <filesystem>
#include <string>

struct OpticalConfig {
  double emissionMinEv;
  double emissionMaxEv;
  double yieldPerMeV;
  double resolutionScale;
  double decayTimeNs;
  double scintillatorIndex;
  double scintillatorAbsorptionCm;
  double airIndex;
  double quantumEfficiency;
};

struct SimulationConfig {
  std::string mode;
  int threads;
  int events;
  int progressInterval;
  std::filesystem::path geometryFile;
  std::filesystem::path visualizationMacro;
  std::string outputFile;
  std::string sourceMode;
  double transitionEnergyMeV;
  double x17MassMeV;
  double x17Fraction;
  std::string gunParticle;
  double gunEnergyMeV;
  std::array<double, 3> gunPositionCm;
  std::array<double, 3> gunDirection;
  OpticalConfig optics;
};

SimulationConfig LoadConfiguration(const std::filesystem::path &file);
