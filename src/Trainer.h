#pragma once

#include "Config.h"
#include "Dataset.h"
#include "Model.h"
#include "Random.h"

// What differs between training phases (pretraining and fine-tuning use different values).
typedef struct TrainerSettings
{
    const char *name;
    int epochs;
    int batchSize;
    float learningRate;
    float learningDecay;
} TrainerSettings;

// Trains the model on the dataset and saves it after every epoch.
// Returns how many optimizer steps were made.
long TrainerRun(const Config *config, const TrainerSettings *settings, Model *model, const Dataset *data, Random *random, const char *modelPath);
