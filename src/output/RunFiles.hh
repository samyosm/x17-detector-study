#pragma once
#include "configuration/Configuration.hh"

int CheckpointEventCount(const Configuration &config);
bool UsesCheckpoints(const Configuration &config);
std::filesystem::path RunOutputFile(const Configuration &config, int runId);
std::filesystem::path PendingRunOutputFile(const Configuration &config, int runId);
void PrepareBatchOutput(const Configuration &config);
void PrepareRunFiles(const Configuration &config, int runId);
void CompleteRunFile(const Configuration &config, int runId);
void MergeCompletedRun(const Configuration &config);
