#pragma once
#include <string>
class Configuration;
class G4VModularPhysicsList;
G4VModularPhysicsList *CreatePhysicsList(const Configuration &config);
std::string DescribePhysics(const Configuration &config);
