#pragma once

#include "network/Model.h"
#include "text/Tokens.h"

// Runs the network one symbol at a time, remembering its hidden state.
typedef struct Inference
{
    const Model *model;
    float *state;
    float *next;
    float *update;
    float *reset;
    float *candidate;
    float *candidateHidden;
    float *scratch;
    float logits[VocabSize];
} Inference;

int InferenceCreate(Inference *inference, const Model *model);
void InferenceStep(Inference *inference, int token);
void InferenceFree(Inference *inference);
