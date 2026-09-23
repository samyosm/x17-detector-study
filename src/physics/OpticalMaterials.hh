#pragma once
class Configuration;
class G4Material;

void ConfigureOpticalMaterials(const Configuration &config,
                               G4Material &scintillator, G4Material &air);
