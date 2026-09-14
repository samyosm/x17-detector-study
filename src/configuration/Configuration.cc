#include "Configuration.hh"

#include <toml++/toml.h>

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace {

template <typename T>
T Required(const toml::table& document, const char* key) {
    const auto value = document.at_path(key).value<T>();
    if (!value) throw std::runtime_error(std::string("Missing or invalid TOML value: ") + key);
    return *value;
}

std::array<double, 3> Vector(const toml::table& document, const char* key) {
    const auto* values = document.at_path(key).as_array();
    if (!values || values->size() != 3) {
        throw std::runtime_error(std::string("Expected three TOML numbers: ") + key);
    }
    std::array<double, 3> result;
    for (std::size_t i = 0; i < result.size(); ++i) {
        const auto value = (*values)[i].value<double>();
        if (!value || !std::isfinite(*value)) {
            throw std::runtime_error(std::string("Invalid TOML vector: ") + key);
        }
        result[i] = *value;
    }
    return result;
}

} // namespace

SimulationConfig LoadConfiguration() {
    const std::filesystem::path file = CONFIG_PATH;
    const auto document = toml::parse_file(file.string());
    const auto resolve = [&file](const std::string& value) {
        if (value.empty()) throw std::runtime_error("Empty configuration path");
        const std::filesystem::path path(value);
        return (path.is_absolute() ? path : file.parent_path() / path).lexically_normal();
    };

    const auto threads = Required<std::int64_t>(document, "runtime.threads");
    const auto events = Required<std::int64_t>(document, "runtime.events");
    const auto progressInterval = Required<std::int64_t>(document, "runtime.progress_interval");
    if (threads < 1 || threads > 1024) {
        throw std::runtime_error("runtime.threads must be between 1 and 1024");
    }
    if (events < 1 || events > 1000000000) {
        throw std::runtime_error("runtime.events must be between 1 and 1000000000");
    }
    if (progressInterval < 1 || progressInterval > 1000000000) {
        throw std::runtime_error("runtime.progress_interval must be between 1 and 1000000000");
    }
    SimulationConfig config{
        Required<std::string>(document, "runtime.mode"),
        static_cast<int>(threads),
        static_cast<int>(events),
        static_cast<int>(progressInterval),
        resolve(Required<std::string>(document, "runtime.geometry_file")),
        resolve(Required<std::string>(document, "runtime.visualization_macro")),
        Required<std::string>(document, "runtime.output_file"),
        Required<std::string>(document, "source.mode"),
        Required<double>(document, "source.transition_energy_mev"),
        Required<double>(document, "source.x17_mass_mev"),
        Required<double>(document, "source.x17_fraction"),
        Required<std::string>(document, "source.gun_particle"),
        Required<double>(document, "source.gun_energy_mev"),
        Vector(document, "source.gun_position_cm"),
        Vector(document, "source.gun_direction"),
        {
            Required<double>(document, "optics.emission_min_ev"),
            Required<double>(document, "optics.emission_max_ev"),
            Required<double>(document, "optics.scintillation_yield_per_mev"),
            Required<double>(document, "optics.resolution_scale"),
            Required<double>(document, "optics.scintillation_decay_ns"),
            Required<double>(document, "optics.scintillator_refractive_index"),
            Required<double>(document, "optics.scintillator_absorption_length_cm"),
            Required<double>(document, "optics.air_refractive_index"),
            Required<double>(document, "optics.glass_refractive_index"),
            Required<double>(document, "optics.pmt_quantum_efficiency")
        }
    };
    if (config.mode != "batch" && config.mode != "visualization") {
        throw std::runtime_error("runtime.mode must be batch or visualization");
    }
    if (config.sourceMode != "gun" && config.sourceMode != "ipc" &&
        config.sourceMode != "x17" && config.sourceMode != "mixed") {
        throw std::runtime_error("source.mode must be gun, ipc, x17, or mixed");
    }
    if (std::filesystem::path(config.outputFile).extension() != ".root") {
        throw std::runtime_error("runtime.output_file must end in .root");
    }
    if (!std::isfinite(config.transitionEnergyMeV) ||
        !std::isfinite(config.x17MassMeV) || !std::isfinite(config.gunEnergyMeV) ||
        config.transitionEnergyMeV <= 0 || config.x17MassMeV <= 0 ||
        config.gunEnergyMeV < 0) {
        throw std::runtime_error("Source energies and mass must be finite and valid");
    }
    if (!std::isfinite(config.x17Fraction) || config.x17Fraction <= 0.0 ||
        config.x17Fraction >= 1.0) {
        throw std::runtime_error("source.x17_fraction must be between 0 and 1");
    }
    const auto& direction = config.gunDirection;
    if (direction[0] == 0 && direction[1] == 0 && direction[2] == 0) {
        throw std::runtime_error("source.gun_direction must be nonzero");
    }
    const auto& optics = config.optics;
    if (!std::isfinite(optics.emissionMinEv) || !std::isfinite(optics.emissionMaxEv) ||
        !std::isfinite(optics.yieldPerMeV) || !std::isfinite(optics.resolutionScale) ||
        !std::isfinite(optics.decayTimeNs) ||
        !std::isfinite(optics.scintillatorIndex) ||
        !std::isfinite(optics.scintillatorAbsorptionCm) ||
        !std::isfinite(optics.airIndex) || !std::isfinite(optics.glassIndex) ||
        !std::isfinite(optics.quantumEfficiency) ||
        optics.emissionMinEv <= 0 || optics.emissionMaxEv <= optics.emissionMinEv ||
        optics.yieldPerMeV <= 0 || optics.resolutionScale <= 0 ||
        optics.decayTimeNs <= 0 ||
        optics.scintillatorIndex <= 0 || optics.scintillatorAbsorptionCm <= 0 ||
        optics.airIndex <= 0 || optics.glassIndex <= 0 ||
        optics.quantumEfficiency <= 0 || optics.quantumEfficiency > 1) {
        throw std::runtime_error("Invalid [optics] values");
    }
    return config;
}
