#pragma once

#include "Dataset.h"
#include "Model.h"
#include "Workspace.h"

typedef struct BatchResult
{
    double loss;
    double weight;
    long tokens;
} BatchResult;

// Splits the listed examples between threads (one workspace per thread).
// With backward set, the summed gradient ends up in spaces[0].gradients.
BatchResult BatchRunnerRun(const Model *model, Workspace *spaces, int threads, const Dataset *data, const int *indices, int count, float questionWeight, float dropout, int backward);
