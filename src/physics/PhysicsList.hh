#pragma once
class Configuration;
class G4VModularPhysicsList;
G4VModularPhysicsList *CreatePhysicsList(const Configuration &config);
