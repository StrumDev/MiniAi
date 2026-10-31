#pragma once

#include "Random.h"

// Writes a slightly damaged copy of a question into output: one word dropped, one letter missing or two letters swapped.
// output needs room for length bytes. Returns the new length, or 0 when the question is too short to damage.
long AugmentQuestion(Random *random, const unsigned char *question, long length, unsigned char *output);
