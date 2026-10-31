#include "Config.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct ConfigEntry
{
    const char *name;
    int isFloat;
    double minimum;
    double maximum;
    void *target;
    const char *description;
} ConfigEntry;

enum
{
    EntryLimit = 32
};

static void ConfigAdd(ConfigEntry *entries, int *count, const char *name, int isFloat, double minimum, double maximum, void *target, const char *description)
{
    ConfigEntry *entry = &entries[*count];
    
    entry->name = name;
    entry->isFloat = isFloat;
    entry->minimum = minimum;
    entry->maximum = maximum;
    entry->target = target;
    entry->description = description;
    
    (*count)++;
}

// The order here is the order of the settings in config.yml and in /config.
static int ConfigEntries(Config *config, ConfigEntry *entries)
{
    int count = 0;
    
    ConfigAdd(entries, &count, "Layers", 0, 1, 16, &config->layers, "Number of GRU layers (changes the architecture)");
    ConfigAdd(entries, &count, "Hidden", 0, 4, 4096, &config->hidden, "Hidden layer size (changes the architecture)");
    ConfigAdd(entries, &count, "Embedding", 0, 2, 1024, &config->embedding, "Character embedding size (changes the architecture)");
    ConfigAdd(entries, &count, "MaxLength", 0, 8, 4096, &config->maxLength, "Max sequence length in bytes (question + answer, or one pretraining text window)");
    ConfigAdd(entries, &count, "Threads", 0, 0, 256, &config->threads, "Number of threads (0 = all CPU cores)");
    ConfigAdd(entries, &count, "Seed", 0, 0, 2147483647.0, &config->seed, "Random seed (0 = new seed every run)");

    ConfigAdd(entries, &count, "WindowStride", 0, 0, 4096, &config->windowStride, "Pretraining: step between text windows in bytes (0 = same as MaxLength, no overlap)");
    ConfigAdd(entries, &count, "PretrainEpochs", 0, 1, 1000000, &config->pretrainEpochs, "Pretraining: number of epochs");
    ConfigAdd(entries, &count, "PretrainBatchSize", 0, 1, 4096, &config->pretrainBatchSize, "Pretraining: mini-batch size");
    ConfigAdd(entries, &count, "PretrainLearningRate", 1, 0.0000001, 1, &config->pretrainLearningRate, "Pretraining: learning rate (Adam)");
    ConfigAdd(entries, &count, "PretrainLearningDecay", 1, 0.01, 1, &config->pretrainLearningDecay, "Pretraining: learning rate multiplier applied after every epoch");

    ConfigAdd(entries, &count, "FinetuneEpochs", 0, 1, 1000000, &config->finetuneEpochs, "Fine-tuning: number of epochs");
    ConfigAdd(entries, &count, "FinetuneBatchSize", 0, 1, 4096, &config->finetuneBatchSize, "Fine-tuning: mini-batch size");
    ConfigAdd(entries, &count, "FinetuneLearningRate", 1, 0.0000001, 1, &config->finetuneLearningRate, "Fine-tuning: learning rate (Adam)");
    ConfigAdd(entries, &count, "FinetuneLearningDecay", 1, 0.01, 1, &config->finetuneLearningDecay, "Fine-tuning: learning rate multiplier applied after every epoch (lets the model settle)");
    ConfigAdd(entries, &count, "FinetuneAugment", 0, 0, 20, &config->finetuneAugment, "Fine-tuning: extra copies of every pair with a damaged question - dropped word, typo (0 = off)");

    ConfigAdd(entries, &count, "ClipNorm", 1, 0, 1000, &config->clipNorm, "Gradient norm clipping (0 = disabled)");
    ConfigAdd(entries, &count, "WeightDecay", 1, 0, 1, &config->weightDecay, "Weight regularization (weight decay)");
    ConfigAdd(entries, &count, "QuestionWeight", 1, 0, 1, &config->questionWeight, "Loss weight on question characters (0 = learn answers only)");
    ConfigAdd(entries, &count, "Dropout", 1, 0, 0.9, &config->dropout, "Dropout between GRU layers while training (0 = off; fights memorizing on small data)");
    ConfigAdd(entries, &count, "ValidationSplit", 1, 0, 0.5, &config->validationSplit, "Share of examples held out for validation (0 = none; a question is held out together with its augmented copies)");
    ConfigAdd(entries, &count, "Patience", 0, 0, 1000, &config->patience, "Early stopping: stop after this many epochs without validation improvement and keep the best model (0 = off; needs ValidationSplit above 0)");
    ConfigAdd(entries, &count, "Normalize", 0, 0, 1, &config->normalize, "Questions: 1 = lowercase and strip punctuation, in training and in chat (0 = keep as typed)");

    ConfigAdd(entries, &count, "ChatHistory", 0, 0, 1, &config->chatHistory, "Chat history: 1 = remember the exchanges of the chat and give the newest ones to the model as context; fine-tuning then also trains with random context, so run /teach finetune after changing it (0 = off)");
    ConfigAdd(entries, &count, "ChatHistoryTurns", 0, 1, 16, &config->chatHistoryTurns, "Chat history: how many of the newest exchanges the model gets as context (also limited by 3/4 of MaxLength bytes)");
    ConfigAdd(entries, &count, "MaxAnswer", 0, 1, 4096, &config->maxAnswer, "Max answer length in bytes");
    ConfigAdd(entries, &count, "Temperature", 1, 0, 5, &config->temperature, "Sampling temperature (0 = always the most likely character)");
    ConfigAdd(entries, &count, "TopK", 0, 0, 256, &config->topK, "Top-K: how many best characters to consider (0 = all)");
    
    return count;
}

void ConfigDefaults(Config *config)
{
    config->layers = 2;
    config->hidden = 128;
    config->embedding = 32;
    config->maxLength = 256;
    config->threads = 0;
    config->seed = 1234;
    config->windowStride = 128;
    config->pretrainEpochs = 300;
    config->pretrainBatchSize = 2;
    config->pretrainLearningRate = 0.005f;
    config->pretrainLearningDecay = 1.0f;
    config->finetuneEpochs = 80;
    config->finetuneBatchSize = 4;
    config->finetuneLearningRate = 0.005f;
    config->finetuneLearningDecay = 0.96f;
    config->finetuneAugment = 3;
    config->clipNorm = 1.0f;
    config->weightDecay = 0.0f;
    config->questionWeight = 0.0f;
    config->dropout = 0.05f;
    config->validationSplit = 0.02f;
    config->patience = 0;
    config->normalize = 1;
    config->chatHistory = 0;
    config->chatHistoryTurns = 3;
    config->maxAnswer = 200;
    config->temperature = 0.2f;
    config->topK = 8;
}

int ConfigNameEquals(const char *left, const char *right)
{
    while (*left != 0 && *right != 0)
    {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right))
            return 0;
        
        left++;
        right++;
    }
    
    return *left == *right;
}

// Names are not case sensitive, so "/set pretrainepochs 100" works as well as "/set PretrainEpochs 100".
int ConfigSet(Config *config, const char *name, const char *value)
{
    ConfigEntry entries[EntryLimit];

    int count = ConfigEntries(config, entries);

    for (int i = 0; i < count; i++)
    {
        if (!ConfigNameEquals(entries[i].name, name))
            continue;

        char *end = 0;
        double number = strtod(value, &end);

        if (end == value || *end != 0)
            return -1;

        if (number < entries[i].minimum || number > entries[i].maximum)
            return -1;

        if (entries[i].isFloat)
        {
            *(float *)entries[i].target = (float)number;
        }
        else
        {
            if (number != floor(number))
                return -1;

            *(int *)entries[i].target = (int)number;
        }

        return 1;
    }

    return 0;
}

static char *ConfigTrim(char *text)
{
    while (*text == ' ' || *text == '\t')
        text++;

    size_t length = strlen(text);

    while (length > 0 && (text[length - 1] == ' ' || text[length - 1] == '\t' || text[length - 1] == '\r' || text[length - 1] == '\n'))
        text[--length] = 0;

    return text;
}

// Reads lines of the form "Name: value". Empty lines and lines starting with # are ignored.
int ConfigLoad(Config *config, const char *path)
{
    FILE *file = fopen(path, "r");
    
    if (file == 0)
        return -1;

    int applied = 0;
    char line[512];

    while (fgets(line, sizeof(line), file) != 0)
    {
        char *text = ConfigTrim(line);
        
        if (text[0] == 0 || text[0] == '#')
            continue;

        char *colon = strchr(text, ':');

        if (colon == 0)
        {
            printf("config: skipped line without ':': %s\n", text);
            continue;
        }

        *colon = 0;

        char *name = ConfigTrim(text);
        char *value = ConfigTrim(colon + 1);
        int status = ConfigSet(config, name, value);

        if (status == 1)
            applied++;

        else if (status == 0)
            printf("config: unknown parameter \"%s\"\n", name);

        else
            printf("config: invalid value \"%s\" for parameter %s\n", value, name);
    }

    fclose(file);
    return applied;
}

static void ConfigWriteValue(FILE *file, const ConfigEntry *entry)
{
    if (entry->isFloat)
        fprintf(file, "%g", (double)*(const float *)entry->target);

    else
        fprintf(file, "%d", *(const int *)entry->target);
}

int ConfigSave(const Config *config, const char *path)
{
    FILE *file = fopen(path, "w");
    
    if (file == 0)
        return 0;

    ConfigEntry entries[EntryLimit];
    int count = ConfigEntries((Config *)config, entries);

    for (int i = 0; i < count; i++)
    {
        fprintf(file, "# %s\n%s: ", entries[i].description, entries[i].name);
        ConfigWriteValue(file, &entries[i]);
        fprintf(file, "\n\n");
    }

    return fclose(file) == 0;
}

void ConfigPrint(const Config *config)
{
    ConfigEntry entries[EntryLimit];
    int count = ConfigEntries((Config *)config, entries);

    for (int i = 0; i < count; i++)
    {
        printf("  %-22s : ", entries[i].name);
        ConfigWriteValue(stdout, &entries[i]);
        printf("\n      %s\n", entries[i].description);
    }
}