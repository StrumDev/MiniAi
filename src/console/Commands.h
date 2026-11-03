#pragma once

#include "config/Context.h"

void CommandHelp(void);
void CommandClear(void);
void CommandConfig(const Context *context);
void CommandSet(Context *context, const char *name, const char *value);
void CommandAsk(Context *context, const char *question);
void CommandContinue(Context *context, const char *text);
void CommandHistoryShow(const Context *context);
void CommandHistoryClear(Context *context);
