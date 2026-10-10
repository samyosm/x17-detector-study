#include "physics/LabSourceDefinition.hh"
#include "G4IonTable.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include <cmath>
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

LabSourceDefinition ReadLabSource(const Configuration &config) {
  const int id = config.Get<int>("source.lab.id");
  const std::string entry = "sources." + std::to_string(id) + ".";
  LabSourceDefinition source;
  source.labId = id;
  source.name = config.Get<std::string>(entry + "name");
  ParseIsotope(source.name, source.atomicNumber, source.massNumber);
  const auto minimum = config.Vector("placement.min_cm");
  const auto maximum = config.Vector("placement.max_cm");
  for (int axis = 0; axis < 3; ++axis)
    if (minimum[axis] > maximum[axis])
      throw std::runtime_error("placement.min_cm must not exceed placement.max_cm");
  source.placementMinimum = {minimum[0] * cm, minimum[1] * cm, minimum[2] * cm};
  source.placementMaximum = {maximum[0] * cm, maximum[1] * cm, maximum[2] * cm};
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
