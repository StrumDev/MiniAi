#pragma once

#include "learning/Dataset.h"
#include "learning/PairList.h"
#include "system/Random.h"

typedef struct PairOptions
{
    int normalize;
    int augment;
    int contextTurns;
    int maxLength;
} PairOptions;

typedef struct PairStats
{
    int pairs;
    int augmented;
    int context;
    int skipped;
} PairStats;

// Builds fine-tuning examples from question/answer pairs.
// normalize: every question is lowercased and stripped of punctuation.
// augment: every pair also gets up to that many copies with a damaged question (dropped word, missing or swapped letter).
// contextTurns above zero: every pair also gets copies that start with 1..contextTurns other pairs as earlier conversation
// (only the final answer is learned), so the model gets used to answering with a chat history in front of the question.
// Copies that would be longer than maxLength are not made.
// Returns 0 when memory runs out.
int DatasetBuildPairs(Dataset *data, const PairList *pairs, const PairOptions *options, Random *random, PairStats *stats);
