#pragma once
class Configuration;
class G4Material;

class G4LogicalVolume;

// Diffusely reflecting wrap on every face of the scintillator bar.
void ConfigureScintillatorWrap(const Configuration &config,
                               G4LogicalVolume &scintillator);

void ConfigureOpticalMaterials(const Configuration &config,
                               G4Material &scintillator, G4Material &air);
