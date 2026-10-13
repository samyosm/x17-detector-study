#pragma once
#include "configuration/Configuration.hh"
#include <optional>
#include <string>

struct SourceOverrides {
  std::string source;
  bool bare = false;
  std::optional<double> x;
  std::optional<double> y;
  std::optional<double> z;
};

void SelectSource(Configuration &config, const SourceOverrides &overrides);
