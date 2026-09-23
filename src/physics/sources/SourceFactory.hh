#pragma once

#include "configuration/Configuration.hh"
#include "physics/sources/PrimarySource.hh"
#include <memory>
std::unique_ptr<PrimarySource> CreatePrimarySource(const Configuration &config);
