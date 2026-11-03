#pragma once

#include "config/Config.h"
#include "learning/Dataset.h"
#include "network/Model.h"
#include "system/Random.h"

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
