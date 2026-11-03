#pragma once

#include "config/Context.h"

// Reads what the user types: lines starting with / are commands, everything else is a question.
void ConsoleRun(Context *context);
