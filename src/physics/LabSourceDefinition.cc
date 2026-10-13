#include "physics/LabSourceDefinition.hh"
#include "G4IonTable.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <random>
#include <vector>
#include <ctime>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace {
constexpr double microcurieInBecquerel = 3.7e4;
constexpr double secondsPerDay = 86400;

std::time_t ParseDate(const std::string &text) {
  std::smatch parts;
  int year, month, day;
  if (std::regex_match(text, parts, std::regex(R"((\d{4})-(\d{2})-(\d{2}))"))) {
    year = std::stoi(parts[1]);
    month = std::stoi(parts[2]);
    day = std::stoi(parts[3]);
  } else if (std::regex_match(text, parts,
                              std::regex(R"((\d{2})-(\d{2})-(\d{4}))"))) {
    day = std::stoi(parts[1]);
    month = std::stoi(parts[2]);
    year = std::stoi(parts[3]);
  } else {
    throw std::runtime_error("Date must be DD-MM-YYYY or YYYY-MM-DD: " + text);
  }
  std::tm date{};
  date.tm_year = year - 1900;
  date.tm_mon = month - 1;
  date.tm_mday = day;
  return timegm(&date);
}

std::string FormatIsoDate(std::time_t time) {
  std::tm date{};
  gmtime_r(&time, &date);
  std::ostringstream text;
  text << std::put_time(&date, "%Y-%m-%d");
  return text.str();
}

std::string TodayIsoDate() { return FormatIsoDate(std::time(nullptr)); }

void ParseIsotope(const std::string &name, int &atomicNumber, int &massNumber) {
  std::smatch parts;
  if (!std::regex_match(name, parts, std::regex(R"(([A-Z][a-z]?)-(\d+))")))
    throw std::runtime_error("Isotope name must look like Cs-137: " + name);
  const auto *element =
      G4NistManager::Instance()->FindOrBuildElement(parts[1].str());
  if (!element)
    throw std::runtime_error("Unknown element in isotope name: " + name);
  atomicNumber = static_cast<int>(element->GetZ());
  massNumber = std::stoi(parts[2]);
}
} // namespace

double DecayCorrectedActivity(double referenceActivityBq, double halfLifeSeconds,
                              double elapsedSeconds) {
  return referenceActivityBq * std::exp2(-elapsedSeconds / halfLifeSeconds);
}

double SecondsBetweenDates(const std::string &from, const std::string &to) {
  return std::difftime(ParseDate(to), ParseDate(from));
}

double PlannedSourceZ(const PlacementSettings &placement, long seed, int runId, int runCount) {
  if (!placement.uniformZ)
    return placement.zFixed;
  std::vector<int> strata(static_cast<std::size_t>(runCount));
  std::iota(strata.begin(), strata.end(), 0);
  std::mt19937_64 order(static_cast<std::uint64_t>(seed));
  std::shuffle(strata.begin(), strata.end(), order);
  std::mt19937_64 jitter(static_cast<std::uint64_t>(seed) * 7919 + static_cast<std::uint64_t>(runId) + 1);
  const double fraction =
      (strata[static_cast<std::size_t>(runId)] + std::uniform_real_distribution<double>(0, 1)(jitter)) / runCount;
  return placement.zMinimum + fraction * (placement.zMaximum - placement.zMinimum);
}

CapsuleSpecification ReadCapsule(const Configuration &config, int labId) {
  const std::string entry = "capsules." + std::to_string(labId) + ".";
  CapsuleSpecification capsule;
  capsule.description = config.Get<std::string>(entry + "description");
  capsule.bodyMaterial = config.Get<std::string>(entry + "body_material");
  capsule.cavityMaterial = config.Get<std::string>(entry + "cavity_material");
  capsule.windowMaterial = config.Get<std::string>(entry + "window_material");
  capsule.activeMaterial = config.Get<std::string>(entry + "active_material");
  capsule.bodyRadius = config.Get<double>(entry + "body_radius_mm") * mm;
  capsule.bodyLength = config.Get<double>(entry + "body_length_mm") * mm;
  capsule.cavityRadius = config.Get<double>(entry + "cavity_radius_mm") * mm;
  capsule.cavityDepth = config.Get<double>(entry + "cavity_depth_mm") * mm;
  capsule.windowThickness = config.Get<double>(entry + "window_thickness_mm") * mm;
  capsule.activeRadius = config.Get<double>(entry + "active_radius_mm") * mm;
  capsule.activeThickness = config.Get<double>(entry + "active_thickness_mm") * mm;
  if (capsule.activeRadius <= 0 || capsule.activeThickness <= 0 ||
      capsule.windowThickness <= 0 || capsule.activeRadius > capsule.cavityRadius ||
      capsule.cavityRadius >= capsule.bodyRadius ||
      capsule.cavityDepth <= capsule.activeThickness ||
      capsule.bodyLength <= capsule.cavityDepth)
    throw std::runtime_error("Inconsistent capsule dimensions for source " + std::to_string(labId));
  return capsule;
}

PlacementSettings ReadPlacement(const Configuration &config) {
  PlacementSettings placement;
  placement.x = config.Get<double>("placement.x_cm") * cm;
  placement.y = config.Get<double>("placement.y_cm") * cm;
  const auto mode = config.Get<std::string>("placement.z_mode");
  if (mode != "uniform" && mode != "fixed")
    throw std::runtime_error("placement.z_mode must be uniform or fixed");
  placement.uniformZ = mode == "uniform";
  placement.zMinimum = config.Get<double>("placement.z_min_cm") * cm;
  placement.zMaximum = config.Get<double>("placement.z_max_cm") * cm;
  placement.zFixed = config.Get<double>("placement.z_cm") * cm;
  if (placement.uniformZ && placement.zMinimum >= placement.zMaximum)
    throw std::runtime_error("placement.z_min_cm must be below placement.z_max_cm");
  const auto direction = config.Vector("placement.orientation");
  placement.orientation = {direction[0], direction[1], direction[2]};
  if (placement.orientation.mag() == 0)
    throw std::runtime_error("placement.orientation must not be zero");
  placement.orientation = placement.orientation.unit();
  return placement;
}

LabSourceDefinition ReadLabSource(const Configuration &config) {
  const int id = config.Get<int>("source.lab.id");
  const std::string entry = "sources." + std::to_string(id) + ".";
  LabSourceDefinition source;
  source.labId = id;
  source.name = config.Get<std::string>(entry + "name");
  ParseIsotope(source.name, source.atomicNumber, source.massNumber);
  source.placement = ReadPlacement(config);
  source.encapsulated = config.Get<bool>("source.lab.encapsulated", true);
  if (source.encapsulated)
    source.capsule = ReadCapsule(config, id);
  source.referenceActivityBq =
      config.Get<double>(entry + "activity_uci") * microcurieInBecquerel;
  source.referenceDate = FormatIsoDate(ParseDate(config.Get<std::string>(entry + "reference_date")));
  source.activityDate = config.Get<std::string>("source.lab.activity_date", TodayIsoDate());
  return source;
}

void ResolveActivity(LabSourceDefinition &source) {
  const auto *ion = G4IonTable::GetIonTable()->GetIon(source.atomicNumber, source.massNumber, 0.0);
  if (!ion || ion->GetPDGStable() || ion->GetPDGLifeTime() <= 0)
    throw std::runtime_error("No radioactive decay data for " + source.name);
  source.halfLifeSeconds = ion->GetPDGLifeTime() * std::log(2.0) / s;
  source.activityBq = DecayCorrectedActivity(
      source.referenceActivityBq, source.halfLifeSeconds,
      SecondsBetweenDates(source.referenceDate, source.activityDate));
}
