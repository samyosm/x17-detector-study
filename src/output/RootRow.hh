#pragma once
#include "G4AnalysisManager.hh"

enum class OutputTree {
  events,
  photonHits,
  cosmicArms,
  cosmicEvents,
  cosmicSteps
};

class RootRow {
public:
  explicit RootRow(OutputTree tree) : tree_(static_cast<int>(tree)) {}
  void Add(int value) {
    G4AnalysisManager::Instance()->FillNtupleIColumn(tree_, column_++, value);
  }
  void Add(double value) {
    G4AnalysisManager::Instance()->FillNtupleDColumn(tree_, column_++, value);
  }
  void Finish() { G4AnalysisManager::Instance()->AddNtupleRow(tree_); }

private:
  int tree_;
  int column_ = 0;
};
