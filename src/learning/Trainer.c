#include "learning/Trainer.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "learning/BatchRunner.h"
#include "system/Interrupt.h"
#include "storage/ModelStorage.h"
#include "network/Optimizer.h"
#include "network/Workspace.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

enum
{
    WarmupSteps = 100,
    ThreadLimit = 256
};

typedef struct TrainerResources
{
    int threads;
    int createdSpaces;
    int adamReady;
    Workspace *spaces;
    Adam adam;
    int *order;
    char *flags;
    float *best;
} TrainerResources;

typedef struct EpochStats
{
    double loss;
    double weight;
    int stopped;
    long skipped;
} EpochStats;

static long TrainerCores(void)
{
#ifdef _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    return (long)info.dwNumberOfProcessors;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

static int TrainerThreads(int requested)
{
    if (requested > 0)
        return requested < ThreadLimit ? requested : ThreadLimit;

    long cores = TrainerCores();

    if (cores < 1)
        return 1;

    return cores < ThreadLimit ? (int)cores : ThreadLimit;
}

static void TrainerDuration(long seconds, char *buffer, int size)
{
    if (seconds < 0)
        seconds = 0;

    snprintf(buffer, (size_t)size, "%02ld:%02ld:%02ld", seconds / 3600, (seconds / 60) % 60, seconds % 60);
}

static int TrainerAcquire(TrainerResources *resources, const Config *config, const Model *model, int total)
{
    memset(resources, 0, sizeof(TrainerResources));

    resources->threads = TrainerThreads(config->threads);
    resources->spaces = (Workspace *)calloc((size_t)resources->threads, sizeof(Workspace));

    if (resources->spaces == 0)
        return 0;

    for (int t = 0; t < resources->threads; t++)
    {
        if (!WorkspaceCreate(&resources->spaces[t], model->layers, model->hidden, model->embedding, config->maxLength))
            return 0;

        resources->createdSpaces++;
    }

    if (!AdamCreate(&resources->adam, model->count))
        return 0;

    resources->adamReady = 1;
    resources->order = (int *)malloc((size_t)total * sizeof(int));
    resources->flags = (char *)malloc((size_t)total);
    resources->best = (float *)malloc((size_t)model->count * sizeof(float));

    return resources->order != 0 && resources->flags != 0 && resources->best != 0;
}

static void TrainerRelease(TrainerResources *resources)
{
    for (int t = 0; t < resources->createdSpaces; t++)
        WorkspaceFree(&resources->spaces[t]);

    free(resources->spaces);

    if (resources->adamReady)
        AdamFree(&resources->adam);

    free(resources->order);
    free(resources->flags);
    free(resources->best);
}

// Puts the validation examples first in resources->order, followed by the training examples.
// A question is held out together with all its augmented copies, so validation never shows a question that training has seen.
static void TrainerSplit(const Dataset *data, Random *random, float split, TrainerResources *resources, int *validationCount, int *trainCount)
{
    int originals = 0;

    for (int i = 0; i < data->count; i++)
    {
        if (data->origins[i] == i)
            resources->order[originals++] = i;
    }

    RandomShuffle(random, resources->order, originals);

    int held = (int)((double)originals * split);

    if (held >= originals)
        held = originals - 1;

    if (held < 0)
        held = 0;

    memset(resources->flags, 0, (size_t)data->count);

    for (int k = 0; k < held; k++)
        resources->flags[resources->order[k]] = 1;

    int used = held;

    for (int i = 0; i < data->count; i++)
    {
        if (!resources->flags[data->origins[i]])
            resources->order[used++] = i;
    }

    *validationCount = held;
    *trainCount = used - held;
}

static void TrainerProgress(int epoch, int epochs, double fraction, double loss, long tokens, long elapsed)
{
    char remaining[32];
    long left = fraction > 0.0 ? (long)((double)elapsed * (1.0 - fraction) / fraction) : 0;
    
    TrainerDuration(left, remaining, sizeof(remaining));
    
    double speed = elapsed > 0 ? (double)tokens / (double)elapsed : 0.0;
    
    printf("\rEpoch %d/%d  %5.1f%%  loss %.4f  %.1fk chars/s  remaining %s   ", epoch, epochs, fraction * 100.0, loss, speed / 1000.0, remaining);
    fflush(stdout);
}

// One pass over the training examples, one optimizer step per mini-batch.
static void TrainerEpoch(const Config *config, const TrainerSettings *settings, Model *model, const Dataset *data, TrainerResources *resources,
    const int *train, int trainCount, int epoch, float rate, long *steps, EpochStats *stats)
{
    double smooth = 0.0;
    long epochTokens = 0;
    long epochStart = (long)time(0);
    long lastPrint = epochStart;

    memset(stats, 0, sizeof(EpochStats));

    for (int start = 0; start < trainCount; start += settings->batchSize)
    {
        int count = trainCount - start < settings->batchSize ? trainCount - start : settings->batchSize;
        
        BatchResult result = BatchRunnerRun(model, resources->spaces, resources->threads, data, train + start, count, config->questionWeight, config->dropout, 1);
        
        epochTokens += result.tokens;
        
        if (result.weight > 0.0)
        {
            float *gradients = resources->spaces[0].gradients;
            
            OptimizerScale(gradients, model->count, (float)(1.0 / result.weight));
            
            float norm = OptimizerClip(gradients, model->count, config->clipNorm);
            
            if (isfinite(norm))
            {
                float warmup = *steps < WarmupSteps ? (float)(*steps + 1) / (float)WarmupSteps : 1.0f;
                AdamStep(&resources->adam, model->params, gradients, rate * warmup, config->weightDecay);
                
                (*steps)++;
                
                double batchLoss = result.loss / result.weight;
                
                stats->loss += result.loss;
                stats->weight += result.weight;
                smooth = smooth == 0.0 ? batchLoss : 0.98 * smooth + 0.02 * batchLoss;
            }
            else
            {
                stats->skipped++;
            }
        }
        
        long now = (long)time(0);
       
        if (now != lastPrint)
        {
            lastPrint = now;
            double fraction = (double)(start + count) / (double)trainCount;
            TrainerProgress(epoch, settings->epochs, fraction, smooth, epochTokens, now - epochStart);
        }

        if (InterruptRequested())
        {
            stats->stopped = 1;
            break;
        }
    }
}

static long TrainerLoop(const Config *config, const TrainerSettings *settings, Model *model, const Dataset *data, Random *random, const char *modelPath, TrainerResources *resources)
{
    int validationCount = 0;
    int trainCount = 0;
    
    TrainerSplit(data, random, config->validationSplit, resources, &validationCount, &trainCount);
    
    int *validation = resources->order;
    int *train = resources->order + validationCount;
    
    for (int t = 0; t < resources->threads; t++)
        RandomSeed(&resources->spaces[t].random, (unsigned long long)RandomNext(random) * 1000003ULL + (unsigned long long)t + 1ULL);

    printf("%s: %ld parameters (%d x GRU %d, embedding %d), threads %d, examples %d (validation %d), dropout %.2f. Ctrl+C stops and saves.\n",
        settings->name, model->count, model->layers, model->hidden, model->embedding, resources->threads, trainCount, validationCount, (double)config->dropout);
    
    if (settings->batchSize < resources->threads)
    {
        printf("Note: the batch size (%d) is smaller than the number of threads (%d), so at most %d of them will work. Raise PretrainBatchSize or FinetuneBatchSize to use all threads.\n",
            settings->batchSize, resources->threads, settings->batchSize);
    }

    int earlyStopping = config->patience > 0 && validationCount > 0;
    
    if (config->patience > 0 && validationCount == 0)
    {
        printf("Note: early stopping is off because there is no validation data. Set ValidationSplit above 0 (for example 0.1).\n");
    }

    long steps = 0;
    long skipped = 0;
    int saved = 0;
    int haveBest = 0;
    int bestEpoch = 0;
    int sinceBest = 0;
    double bestLoss = 0.0;
    int finished = 0;
    long runStart = (long)time(0);

    InterruptArm();

    for (int epoch = 1; epoch <= settings->epochs && !finished; epoch++)
    {
        float rate = settings->learningRate * (float)pow((double)settings->learningDecay, (double)(epoch - 1));
        
        RandomShuffle(random, train, trainCount);
        
        EpochStats stats;
        
        TrainerEpoch(config, settings, model, data, resources, train, trainCount, epoch, rate, &steps, &stats);
        skipped += stats.skipped;

        double validationLoss = -1.0;
        
        if (validationCount > 0 && !stats.stopped)
        {
            BatchResult check = BatchRunnerRun(model, resources->spaces, resources->threads, data, validation, validationCount, config->questionWeight, 0.0f, 0);
            
            if (check.weight > 0.0)
                validationLoss = check.loss / check.weight;
        }

        char elapsedText[32];
        TrainerDuration((long)time(0) - runStart, elapsedText, sizeof(elapsedText));

        double average = stats.weight > 0.0 ? stats.loss / stats.weight : 0.0;
        printf("\rEpoch %d/%d%s: loss %.4f (perplexity %.2f)", epoch, settings->epochs, stats.stopped ? " interrupted" : " finished", average, exp(average));
        
        if (validationLoss >= 0.0)
            printf(", validation %.4f", validationLoss);

        printf(", learning rate %.5f, time %s                    \n", (double)rate, elapsedText);

        if (stats.stopped)
            finished = 1;

        if (earlyStopping)
        {
            if (validationLoss >= 0.0 && (!haveBest || validationLoss < bestLoss))
            {
                haveBest = 1;
                bestLoss = validationLoss;
                bestEpoch = epoch;
                sinceBest = 0;

                memcpy(resources->best, model->params, (size_t)model->count * sizeof(float));
                saved = ModelStorageSave(model, modelPath);
            }
            else if (validationLoss >= 0.0)
            {
                sinceBest++;

                if (sinceBest >= config->patience)
                {
                    printf("Early stopping: validation has not improved for %d epochs.\n", sinceBest);
                    finished = 1;
                }
            }
        }
        else if (steps > 0)
        {
            saved = ModelStorageSave(model, modelPath);

            if (!saved)
                printf("Could not save the model to %s\n", modelPath);
        }
    }

    InterruptDisarm();

    if (earlyStopping)
    {
        if (haveBest)
        {
            memcpy(model->params, resources->best, (size_t)model->count * sizeof(float));
            printf("Kept the best model: epoch %d, validation %.4f.\n", bestEpoch, bestLoss);
        }
        else if (steps > 0)
        {
            saved = ModelStorageSave(model, modelPath);
        }
    }

    if (saved)
        printf("Model saved: %s\n", modelPath);

    if (skipped > 0)
        printf("Steps skipped because of an unstable gradient: %ld (try a smaller learning rate)\n", skipped);

    return steps;
}

long TrainerRun(const Config *config, const TrainerSettings *settings, Model *model, const Dataset *data, Random *random, const char *modelPath)
{
    TrainerResources resources;
    long steps = 0;

    if (TrainerAcquire(&resources, config, model, data->count))
        steps = TrainerLoop(config, settings, model, data, random, modelPath, &resources);
    else
        printf("Not enough memory for training. Reduce Hidden, Layers, MaxLength or Threads.\n");

    TrainerRelease(&resources);
    return steps;
}