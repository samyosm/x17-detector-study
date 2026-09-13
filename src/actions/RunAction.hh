#pragma once

#include "G4UserRunAction.hh"

#include <string>

class RunAction final : public G4UserRunAction {
public:
    explicit RunAction(std::string outputFile);
    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

private:
    std::string outputFile_;
};
