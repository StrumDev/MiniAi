#pragma once

#include "chat/ChatHistory.h"
#include "config/Config.h"
#include "network/Model.h"
#include "system/Random.h"

// Feeds the question and the answer marker to the network, then writes the generated answer as text.
// With ChatHistory switched on in the config, the newest exchanges of the history go in front of the question
// and the new exchange is added to the history. Returns 0 on failure.
int ResponderAnswer(const Model *model, const Config *config, Random *random, ChatHistory *history, const char *question, char *output, int outputSize);

// Feeds plain text to the network and writes how it would continue that text. Returns 0 on failure.
int ResponderContinue(const Model *model, const Config *config, Random *random, const char *text, char *output, int outputSize);
