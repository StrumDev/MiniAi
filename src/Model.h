#pragma once

#include "Layout.h"
#include "Random.h"

typedef struct Model
{
    int layers;
    int hidden;
    int embedding;
    long count;
    float *params;
    Layout weights;
} Model;

int ModelCreate(Model *model, int layers, int hidden, int embedding);
void ModelInitialize(Model *model, Random *random);
int ModelMatches(const Model *model, int layers, int hidden, int embedding);
void ModelFree(Model *model);
