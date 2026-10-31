#pragma once

typedef struct Config
{
    int layers;
    int hidden;
    int embedding;
    int maxLength;
    int finetuneBatchSize;
    int finetuneEpochs;
    int threads;
    int seed;
    int maxAnswer;
    int topK;
    int patience;
    int normalize;
    int finetuneAugment;
    int chatHistory;
    int chatHistoryTurns;
    int windowStride;
    int pretrainEpochs;
    int pretrainBatchSize;
    float finetuneLearningRate;
    float pretrainLearningRate;
    float pretrainLearningDecay;
    float finetuneLearningDecay;
    float clipNorm;
    float weightDecay;
    float questionWeight;
    float validationSplit;
    float temperature;
    float dropout;
} Config;

void ConfigDefaults(Config *config);

// Returns how many keys were applied, or -1 when the file cannot be opened.
int ConfigLoad(Config *config, const char *path);

int ConfigSave(const Config *config, const char *path);

// Returns 1 when applied, 0 for an unknown key, -1 for a value that is not allowed.
int ConfigSet(Config *config, const char *name, const char *value);

void ConfigPrint(const Config *config);

// Compares two key names ignoring letter case. Returns 1 when they are equal.
int ConfigNameEquals(const char *left, const char *right);
