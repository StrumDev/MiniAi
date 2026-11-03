#include <stdio.h>
#include <string.h>
#include <time.h>
#include "config/Config.h"
#include "console/Console.h"
#include "config/Context.h"
#include "storage/FileList.h"
#include "storage/Locator.h"
#include "storage/ModelStorage.h"

#ifdef _WIN32
#include <windows.h>
#endif

static void MainLocateFiles(Context *context)
{
    if (LocatorFind("config.yml", context->programPath, context->configPath, PathSize))
        ConfigLoad(&context->config, context->configPath);
    else
        snprintf(context->configPath, PathSize, "config.yml");

    LocatorFindFolder("training", context->programPath, context->trainingFolder, PathSize);

    if (!LocatorFind("model.bin", context->programPath, context->modelPath, PathSize))
    {
        char directory[PathSize];

        LocatorDirectoryOf(context->configPath, directory, PathSize);
        snprintf(context->modelPath, PathSize, "%s/model.bin", directory);
    }
}

static void MainReportTraining(const Context *context)
{
    if (context->trainingFolder[0] == 0)
    {
        printf("  training folder: not found (create a folder named training with .pre and .fin files)\n");
        return;
    }

    FileList pretrain;
    FileList finetune;

    FileListScan(&pretrain, context->trainingFolder, ".pre");
    FileListScan(&finetune, context->trainingFolder, ".fin");
    
    printf("  training folder: %s (%d .pre files, %d .fin files)\n", context->trainingFolder, pretrain.count, finetune.count);
    
    FileListFree(&pretrain);
    FileListFree(&finetune);
}

static void MainReport(const Context *context)
{
    printf("MiniAi - a neural network (GRU) in C\n");
    printf("  settings: %s\n", context->configPath);
    
    MainReportTraining(context);
    
    if (context->modelReady)
        printf("  model: %s (%d x GRU %d, %ld parameters)\n", context->modelPath, context->model.layers, context->model.hidden, context->model.count);
    else
        printf("  model: not trained yet - type /teach\n");
    
    if (context->config.chatHistory)
        printf("  chat history: on (the last %d exchanges are given to the model)\n", context->config.chatHistoryTurns);
    else
        printf("  chat history: off (set ChatHistory: 1 in config.yml to turn it on)\n");
    
    printf("Help: /help\n");
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    Context context;
    memset(&context, 0, sizeof(Context));

    if (argc > 0)
        snprintf(context.programPath, PathSize, "%s", argv[0]);

    ConfigDefaults(&context.config);
    MainLocateFiles(&context);

    unsigned long long seed = context.config.seed != 0 ? (unsigned long long)context.config.seed : (unsigned long long)time(0);
    RandomSeed(&context.random, seed);

    context.modelReady = ModelStorageLoad(&context.model, context.modelPath);

    MainReport(&context);
    ConsoleRun(&context);
    ChatHistoryClear(&context.history);

    if (context.modelReady)
        ModelFree(&context.model);

    return 0;
}