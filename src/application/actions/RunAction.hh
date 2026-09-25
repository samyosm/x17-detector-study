#pragma once

#include "G4UserRunAction.hh"

#include "configuration/Configuration.hh"
#include <optional>

class RunAction final : public G4UserRunAction {
public:
    explicit RunAction(const Configuration& config);
    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

private:
    Configuration config_;
    std::optional<double> cosmicRateHz_;
};
