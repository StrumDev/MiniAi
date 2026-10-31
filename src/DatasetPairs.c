#include "DatasetPairs.h"
#include <stdlib.h>
#include <string.h>
#include "Augment.h"
#include "Normalize.h"
#include "Tokens.h"

enum
{
    ContextCopies = 2,
    ContextLimit = 16
};

// Everything the builder needs while it walks over the pairs.
typedef struct PairBuilder
{
    Dataset *data;
    const PairList *pairs;
    const PairOptions *options;
    Random *random;
    unsigned char **questions;
    long *questionLengths;
    int *valid;
    int validCount;
    unsigned char *buffer;
    unsigned char *variant;
} PairBuilder;

static long PairBuilderAppend(unsigned char *buffer, long used, const unsigned char *bytes, long length)
{
    for (long i = 0; i < length; i++)
        buffer[used + i] = bytes[i] < FirstTextByte ? ' ' : bytes[i];

    return used + length;
}

// Appends one turn: question, Separator, answer, EndMark.
static long PairBuilderTurn(unsigned char *buffer, long used, const unsigned char *question, long questionLength, const unsigned char *answer, long answerLength)
{
    used = PairBuilderAppend(buffer, used, question, questionLength);
    buffer[used++] = Separator;
    
    used = PairBuilderAppend(buffer, used, answer, answerLength);
    buffer[used++] = EndMark;
    
    return used;
}

static long PairBuilderTurnLength(const PairBuilder *builder, int index)
{
    return builder->questionLengths[index] + builder->pairs->answerLengths[index] + 2;
}

// Tells whether one of the examples first..count-1 already has exactly this question.
static int PairBuilderKnown(const Dataset *data, int first, const unsigned char *question, long length)
{
    for (int i = first; i < data->count; i++)
    {
        if (data->separators[i] == length && memcmp(DatasetTokens(data, i), question, (size_t)length) == 0)
            return 1;
    }

    return 0;
}

static int PairBuilderOriginal(PairBuilder *builder, int index)
{
    long total = PairBuilderTurn(builder->buffer, 0, builder->questions[index], builder->questionLengths[index], builder->pairs->answers[index], builder->pairs->answerLengths[index]);
    return DatasetAdd(builder->data, builder->buffer, total, (int)builder->questionLengths[index], -1);
}

// Adds one copy with a damaged question. Returns 1 when a copy was added, 0 when none could be made, -1 when memory ran out.
static int PairBuilderDamaged(PairBuilder *builder, int index, int origin)
{
    long questionLength = builder->questionLengths[index];
    long variantLength = AugmentQuestion(builder->random, builder->questions[index], questionLength, builder->variant);
    
    if (variantLength <= 0 || PairBuilderKnown(builder->data, origin, builder->variant, variantLength))
        return 0;

    long total = PairBuilderTurn(builder->buffer, 0, builder->variant, variantLength, builder->pairs->answers[index], builder->pairs->answerLengths[index]);
    return DatasetAdd(builder->data, builder->buffer, total, (int)variantLength, origin) ? 1 : -1;
}

// Adds one copy that starts with other pairs as earlier conversation. Returns 1 when added, 0 when none could be made, -1 when memory ran out.
static int PairBuilderContext(PairBuilder *builder, int index, int position, int origin)
{
    int turns = builder->options->contextTurns < ContextLimit ? builder->options->contextTurns : ContextLimit;
    
    if (builder->validCount < 2)
        return 0;

    int chosen[ContextLimit];
    int wanted = 1 + (int)(RandomNext(builder->random) % (unsigned int)turns);

    for (int k = 0; k < wanted; k++)
    {
        int pick = (int)(RandomNext(builder->random) % (unsigned int)(builder->validCount - 1));
        chosen[k] = builder->valid[pick >= position ? pick + 1 : pick];
    }

    long total = PairBuilderTurnLength(builder, index);

    for (int k = 0; k < wanted; k++)
        total += PairBuilderTurnLength(builder, chosen[k]);

    while (wanted > 0 && total > builder->options->maxLength)
    {
        wanted--;
        total -= PairBuilderTurnLength(builder, chosen[wanted]);
    }

    if (wanted == 0)
        return 0;

    long used = 0;

    for (int k = 0; k < wanted; k++)
    {
        int other = chosen[k];
        used = PairBuilderTurn(builder->buffer, used, builder->questions[other], builder->questionLengths[other], builder->pairs->answers[other], builder->pairs->answerLengths[other]);
    }

    int separator = (int)(used + builder->questionLengths[index]);
    used = PairBuilderTurn(builder->buffer, used, builder->questions[index], builder->questionLengths[index], builder->pairs->answers[index], builder->pairs->answerLengths[index]);
    
    return DatasetAdd(builder->data, builder->buffer, used, separator, origin) ? 1 : -1;
}

// Cleans every question and remembers which pairs are usable. Returns 0 when memory runs out.
static int PairBuilderPrepare(PairBuilder *builder, PairStats *stats)
{
    const PairList *pairs = builder->pairs;
    long longest = builder->options->maxLength;
    
    builder->questions = (unsigned char **)calloc((size_t)pairs->count + 1, sizeof(unsigned char *));
    builder->questionLengths = (long *)calloc((size_t)pairs->count + 1, sizeof(long));
    builder->valid = (int *)malloc(((size_t)pairs->count + 1) * sizeof(int));
    
    if (builder->questions == 0 || builder->questionLengths == 0 || builder->valid == 0)
        return 0;

    for (int i = 0; i < pairs->count; i++)
    {
        long length = pairs->questionLengths[i];
        unsigned char *question = (unsigned char *)malloc((size_t)length + 1);
        
        if (question == 0)
            return 0;

        if (builder->options->normalize)
            length = NormalizeQuestion(pairs->questions[i], length, question);
        else
            memcpy(question, pairs->questions[i], (size_t)length);

        builder->questions[i] = question;
        builder->questionLengths[i] = length;

        if (length == 0)
        {
            stats->skipped++;
            continue;
        }

        builder->valid[builder->validCount++] = i;

        long turn = length + pairs->answerLengths[i] + 2;
        longest = turn > longest ? turn : longest;
    }

    builder->buffer = (unsigned char *)malloc((size_t)longest + 8);
    builder->variant = (unsigned char *)malloc((size_t)longest + 8);

    return builder->buffer != 0 && builder->variant != 0;
}

static void PairBuilderFree(PairBuilder *builder)
{
    if (builder->questions != 0)
    {
        for (int i = 0; i < builder->pairs->count; i++)
            free(builder->questions[i]);
    }

    free(builder->questions);
    free(builder->questionLengths);
    free(builder->valid);
    free(builder->buffer);
    free(builder->variant);
}

int DatasetBuildPairs(Dataset *data, const PairList *pairs, const PairOptions *options, Random *random, PairStats *stats)
{
    memset(stats, 0, sizeof(PairStats));

    PairBuilder builder;

    memset(&builder, 0, sizeof(PairBuilder));

    builder.data = data;
    builder.pairs = pairs;
    builder.options = options;
    builder.random = random;

    int ok = PairBuilderPrepare(&builder, stats);

    for (int position = 0; ok && position < builder.validCount; position++)
    {
        int index = builder.valid[position];
        int origin = data->count;

        ok = PairBuilderOriginal(&builder, index);
        stats->pairs++;

        for (int copy = 0; ok && copy < options->augment; copy++)
        {
            int result = PairBuilderDamaged(&builder, index, origin);

            ok = result >= 0;
            stats->augmented += result > 0 ? 1 : 0;
        }

        for (int copy = 0; ok && options->contextTurns > 0 && copy < ContextCopies; copy++)
        {
            int result = PairBuilderContext(&builder, index, position, origin);

            ok = result >= 0;
            stats->context += result > 0 ? 1 : 0;
        }
    }

    PairBuilderFree(&builder);
    return ok;
}