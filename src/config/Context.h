#pragma once

#include "chat/ChatHistory.h"
#include "config/Config.h"
#include "storage/Locator.h"
#include "network/Model.h"
#include "system/Random.h"

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
