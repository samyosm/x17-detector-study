#pragma once
class Configuration;
class G4Material;

// optics.mode: "tracking" follows every optical photon; "light_map" draws the PMT photon counts
// from a table of collection probabilities built with --build-light-map.
bool TracksOpticalPhotons(const Configuration &config);
bool UsesLightMap(const Configuration &config);
// In tracking mode the quantum efficiency can be folded into the light yield: the detected
// count has the same statistics and only the photons that will be detected are tracked.
bool FoldsEfficiencyIntoYield(const Configuration &config);

class G4LogicalVolume;

// Diffusely reflecting wrap on every face of the scintillator bar.
void ConfigureScintillatorWrap(const Configuration &config,
                               G4LogicalVolume &scintillator);

void ConfigureOpticalMaterials(const Configuration &config,
                               G4Material &scintillator, G4Material &air);
