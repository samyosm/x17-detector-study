#pragma once

#include <filesystem>
#include <array>
#include <string>

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
};

SimulationConfig LoadConfiguration();
