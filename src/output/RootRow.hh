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
  explicit RootRow(OutputTree tree)
      : output_(G4AnalysisManager::Instance()), tree_(static_cast<int>(tree)) {}
  void Add(int value) {
    output_->FillNtupleIColumn(tree_, column_++, value);
  }
  void Add(double value) {
    output_->FillNtupleDColumn(tree_, column_++, value);
  }
  void Finish() { output_->AddNtupleRow(tree_); }

private:
  G4AnalysisManager *output_;
  int tree_;
  int column_ = 0;
};
