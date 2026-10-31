#pragma once

#include "Random.h"

// Picks the next symbol from raw network outputs.
// temperature <= 0.01 gives the most likely symbol; topK of 0 keeps every symbol.
int SamplerPick(const float *logits, int count, float temperature, int topK, Random *random);
