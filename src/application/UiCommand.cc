#include "application/UiCommand.hh"
#include "G4UImanager.hh"
#include <stdexcept>

void ExecuteUiCommand(const std::string& command) {
    if (G4UImanager::GetUIpointer()->ApplyCommand(command) != 0)
        throw std::runtime_error("Geant4 command failed: " + command);
}
