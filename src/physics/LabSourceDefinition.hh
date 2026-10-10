#pragma once
#include "G4ThreeVector.hh"
#include "configuration/Configuration.hh"
#include <string>

struct LabSourceDefinition {
  int labId;
  std::string name;
  int atomicNumber;
  int massNumber;
  G4ThreeVector placementMinimum;
  G4ThreeVector placementMaximum;
  double referenceActivityBq;
  std::string referenceDate;
  std::string activityDate;
  double halfLifeSeconds = 0;
  double activityBq = 0;
};

double DecayCorrectedActivity(double referenceActivityBq, double halfLifeSeconds,
                              double elapsedSeconds);
double SecondsBetweenDates(const std::string &from, const std::string &to);
LabSourceDefinition ReadLabSource(const Configuration &config);
void ResolveActivity(LabSourceDefinition &source);
