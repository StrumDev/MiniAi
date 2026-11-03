#pragma once

#include "network/Layout.h"
#include "system/Random.h"

// Everything one thread needs to run a sequence forward and backward.
typedef struct Workspace
{
    int layers;
    int hidden;
    int embedding;
    int maxLength;
    float *update;
    float *reset;
    float *candidate;
    float *candidateHidden;
    float *state;
    float *drop;
    float *mask;
    float *gradTop;
    float *zeros;
    float *forwardScratch;
    float *backwardScratch;
    float *carry;
    float *sum;
    float *gradInput;
    float *probs;
    float *gradients;
    long gradCount;
    Layout grads;
    float dropout;
    Random random;
    double loss;
    double weight;
    long tokens;
} Workspace;

int WorkspaceCreate(Workspace *space, int layers, int hidden, int embedding, int maxLength);
void WorkspaceReset(Workspace *space, int clearGradients);
void WorkspaceFree(Workspace *space);

// Cache slot of one layer at one time step inside an array shaped [layers][maxLength][hidden].
static inline float *WorkspaceSlot(const Workspace *space, float *array, int layer, int step)
{
    return array + ((long)layer * space->maxLength + step) * space->hidden;
}
