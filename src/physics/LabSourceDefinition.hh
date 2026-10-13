#pragma once
#include "G4ThreeVector.hh"
#include "configuration/Configuration.hh"
#include <string>

struct CapsuleSpecification {
  std::string description;
  std::string bodyMaterial;
  std::string cavityMaterial;
  std::string windowMaterial;
  std::string activeMaterial;
  double bodyRadius;
  double bodyLength;
  double cavityRadius;
  double cavityDepth;
  double windowThickness;
  double activeRadius;
  double activeThickness;
};

struct PlacementSettings {
  double x;
  double y;
  bool uniformZ;
  double zMinimum;
  double zMaximum;
  double zFixed;
  G4ThreeVector orientation;
};

struct LabSourceDefinition {
  int labId;
  std::string name;
  int atomicNumber;
  int massNumber;
  PlacementSettings placement;
  bool encapsulated;
  CapsuleSpecification capsule;
  double referenceActivityBq;
  std::string referenceDate;
  std::string activityDate;
  double halfLifeSeconds = 0;
  double activityBq = 0;
};

double DecayCorrectedActivity(double referenceActivityBq, double halfLifeSeconds,
                              double elapsedSeconds);
double SecondsBetweenDates(const std::string &from, const std::string &to);
double PlannedSourceZ(const PlacementSettings &placement, long seed, int runId, int runCount);
CapsuleSpecification ReadCapsule(const Configuration &config, int labId);
PlacementSettings ReadPlacement(const Configuration &config);
LabSourceDefinition ReadLabSource(const Configuration &config);
void ResolveActivity(LabSourceDefinition &source);
