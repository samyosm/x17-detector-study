#pragma once
#include "G4VUserActionInitialization.hh"
#include "source/SourceSettings.hh"
#include <memory>
#include <string>

class ActionInitialization final : public G4VUserActionInitialization {
public:
    ActionInitialization(std::shared_ptr<const SourceSettings> settings,
                         std::string outputFile, int progressInterval);
    void BuildForMaster() const override;
    void Build() const override;

private:
    std::shared_ptr<const SourceSettings> settings_;
    std::string outputFile_;
    int progressInterval_;
};
