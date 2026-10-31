#pragma once

#include "ChatHistory.h"
#include "Config.h"
#include "Locator.h"
#include "Model.h"
#include "Random.h"

// Everything the running program shares between startup, commands and the console.
typedef struct Context
{
    Config config;
    Model model;
    int modelReady;
    Random random;
    ChatHistory history;
    char programPath[PathSize];
    char configPath[PathSize];
    char trainingFolder[PathSize];
    char modelPath[PathSize];
} Context;
