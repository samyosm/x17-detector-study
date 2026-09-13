#pragma once

#include <string>
#include "G4ThreeVector.hh"

struct SourceSettings {
    std::string mode;
    double transitionEnergy;
    double x17Mass;
    std::string gunParticle;
    double gunEnergy;
    G4ThreeVector gunPosition;
    G4ThreeVector gunDirection;
};
