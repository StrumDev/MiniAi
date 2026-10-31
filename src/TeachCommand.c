#include "TeachCommand.h"
#include <stdio.h>
#include "DatasetPairs.h"
#include "DatasetText.h"
#include "FileList.h"
#include "FinParser.h"
#include "Interrupt.h"
#include "ModelStorage.h"
#include "PairList.h"
#include "Trainer.h"

// Reads every .pre file of the training folder into one dataset of text windows.
// Returns 1 when there is something to learn from.
static int TeachLoadText(const Context *context, Dataset *data)
{
    const Config *config = &context->config;

    FileList files;

    if (context->trainingFolder[0] == 0)
    {
        printf("The training folder was not found (searched the current folder, two parent folders, data/ and next to the program). No pretraining data.\n");
        return 0;
    }

    if (!FileListScan(&files, context->trainingFolder, ".pre") || files.count == 0)
    {
        printf("No .pre files in %s. No pretraining data.\n", context->trainingFolder);
        FileListFree(&files);
        return 0;
    }

    if (!DatasetInit(data))
    {
        printf("Not enough memory for the data.\n");
        FileListFree(&files);
        return 0;
    }

    long totalBytes = 0;

    for (int i = 0; i < files.count; i++)
    {
        long bytes = 0;
        int before = data->count;

        if (!DatasetAddText(data, files.paths[i], config->maxLength, config->windowStride, &bytes))
        {
            printf("  Could not read %s\n", files.paths[i]);
            continue;
        }

        printf("  %s: %ld bytes, %d windows\n", files.paths[i], bytes, data->count - before);
        totalBytes += bytes;
    }

    printf("Pretraining data: %d files, %ld bytes of text, %d windows of up to %d bytes\n", files.count, totalBytes, data->count, config->maxLength);
    FileListFree(&files);

    if (data->count == 0)
    {
        printf("The .pre files have nothing to learn from.\n");
        DatasetFree(data);
        return 0;
    }

    return 1;
}

// Reads every .fin file of the training folder and builds the fine-tuning examples.
// Returns 1 when there is something to learn from.
static int TeachLoadPairs(Context *context, Dataset *data)
{
    const Config *config = &context->config;

    FileList files;

    if (context->trainingFolder[0] == 0)
    {
        printf("The training folder was not found (searched the current folder, two parent folders, data/ and next to the program). No fine-tuning data.\n");
        return 0;
    }
    
    if (!FileListScan(&files, context->trainingFolder, ".fin") || files.count == 0)
    {
        printf("No .fin files in %s. No fine-tuning data.\n", context->trainingFolder);
        FileListFree(&files);
        return 0;
    }

    PairList pairs;
    PairListInit(&pairs);

    int errors = 0;

    for (int i = 0; i < files.count; i++)
    {
        int added = 0;
        int fileErrors = 0;

        if (!FinParseFile(files.paths[i], &pairs, &added, &fileErrors))
        {
            printf("  Could not read %s\n", files.paths[i]);
            continue;
        }

        printf("  %s: %d pairs, invalid entries: %d\n", files.paths[i], added, fileErrors);
        errors += fileErrors;
    }

    int fileCount = files.count;
    FileListFree(&files);

    PairOptions options;
    options.normalize = config->normalize;
    options.augment = config->finetuneAugment;
    options.contextTurns = config->chatHistory ? config->chatHistoryTurns : 0;
    options.maxLength = config->maxLength;

    PairStats stats;
    int built = DatasetInit(data) && DatasetBuildPairs(data, &pairs, &options, &context->random, &stats);

    PairListFree(&pairs);

    if (!built)
    {
        printf("Not enough memory for the data.\n");
        DatasetFree(data);
        return 0;
    }

    printf("Fine-tuning data: %d files, %d pairs + %d augmented copies + %d copies with chat context, invalid entries: %d\n",
        fileCount, stats.pairs, stats.augmented, stats.context, errors + stats.skipped);

    if (data->count == 0)
    {
        printf("The .fin files have no valid pairs.\n");
        DatasetFree(data);

        return 0;
    }

    return 1;
}

static int TeachLoad(Context *context, int phase, Dataset *data)
{
    return phase == PhasePretrain ? TeachLoadText(context, data) : TeachLoadPairs(context, data);
}

// Makes sure context->model is the model to train. Returns 1 when ready; created tells if it is new.
static int TeachPrepare(Context *context, int fresh, int *created)
{
    Config *config = &context->config;

    int compatible = context->modelReady && ModelMatches(&context->model, config->layers, config->hidden, config->embedding);
    
    *created = 0;
    
    if (!fresh && compatible)
    {
        printf("Continuing to train the saved model.\n");
        return 1;
    }

    if (context->modelReady && !fresh)
        printf("The architecture in the settings differs from the saved model - training starts from scratch.\n");

    if (context->modelReady)
    {
        ModelFree(&context->model);
        context->modelReady = 0;
    }

    if (!ModelCreate(&context->model, config->layers, config->hidden, config->embedding))
    {
        printf("Not enough memory for the model.\n");
        return 0;
    }

    ModelInitialize(&context->model, &context->random);

    context->modelReady = 1;
    *created = 1;
    return 1;
}

static TrainerSettings TeachSettings(const Config *config, int phase)
{
    TrainerSettings settings;

    if (phase == PhasePretrain)
    {
        settings.name = "Pretraining";
        settings.epochs = config->pretrainEpochs;
        settings.batchSize = config->pretrainBatchSize;
        settings.learningRate = config->pretrainLearningRate;
        settings.learningDecay = config->pretrainLearningDecay;
    }
    else
    {
        settings.name = "Fine-tuning";
        settings.epochs = config->finetuneEpochs;
        settings.batchSize = config->finetuneBatchSize;
        settings.learningRate = config->finetuneLearningRate;
        settings.learningDecay = config->finetuneLearningDecay;
    }

    return settings;
}

static void TeachNothing(void)
{
    printf("Nothing to train on. Put plain text into .pre files and \"in:\"/\"out:\" pairs into .fin files in the training folder.\n");
}

// Trains on the loaded data; drops a model that was created for this run but never learned anything.
static void TeachTrain(Context *context, const Dataset *data, const TrainerSettings *settings, int fresh)
{
    int created = 0;

    if (!TeachPrepare(context, fresh, &created))
        return;

    long steps = TrainerRun(&context->config, settings, &context->model, data, &context->random, context->modelPath);
    
    if (steps == 0 && created)
    {
        ModelFree(&context->model);
        context->modelReady = 0;
    }
}

// Plain text windows and in:/out: pairs are shuffled together and learned in a single pass.
static void TeachMixed(Context *context, int fresh)
{
    Dataset text;
    Dataset pairs;

    int haveText = TeachLoad(context, PhasePretrain, &text);
    int havePairs = TeachLoad(context, PhaseFinetune, &pairs);

    if (!haveText && !havePairs)
    {
        TeachNothing();
        return;
    }

    Dataset *target = havePairs ? &pairs : &text;

    if (haveText && havePairs)
    {
        int merged = DatasetMerge(&pairs, &text);
        DatasetFree(&text);

        if (!merged)
        {
            printf("Not enough memory for the data.\n");
            DatasetFree(&pairs);
            return;
        }
    }

    TrainerSettings settings = TeachSettings(&context->config, PhaseFinetune);
    settings.name = "Mixed training";

    TeachTrain(context, target, &settings, fresh);
    DatasetFree(target);
}

void CommandTeach(Context *context, int phases, int fresh)
{
    if (phases & PhaseMixed)
    {
        TeachMixed(context, fresh);
        return;
    }

    int loaded = 0;
    int freshNow = fresh;

    for (int phase = PhasePretrain; phase <= PhaseFinetune; phase <<= 1)
    {
        if ((phases & phase) == 0)
            continue;

        Dataset data;

        if (!TeachLoad(context, phase, &data))
            continue;

        loaded++;

        TrainerSettings settings = TeachSettings(&context->config, phase);

        TeachTrain(context, &data, &settings, freshNow);

        freshNow = 0;

        DatasetFree(&data);

        if (InterruptRequested())
            break;
    }

    if (loaded == 0)
        TeachNothing();
}