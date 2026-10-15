#pragma once
#include "configuration/Configuration.hh"

// Tracks optical photons from a grid of points in a scintillator bar and stores, per cell, the
// fraction that reaches each end face. Writes the file named by optics.light_map.file.
void BuildLightMap(Configuration config);
