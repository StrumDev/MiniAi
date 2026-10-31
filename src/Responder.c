#include "Responder.h"
#include <stdlib.h>
#include <string.h>
#include "Inference.h"
#include "Normalize.h"
#include "Sampler.h"
#include "Tokens.h"
#include "Utf8.h"

static void ResponderFeedBytes(Inference *inference, const unsigned char *bytes, long length)
{
    for (long i = 0; i < length; i++)
        InferenceStep(inference, bytes[i]);
}

static void ResponderFeed(Inference *inference, const char *text)
{
    for (const char *cursor = text; *cursor != 0; cursor++)
    {
        unsigned char byte = (unsigned char)*cursor;
        InferenceStep(inference, byte < FirstTextByte ? ' ' : byte);
    }
}

// Samples symbols one by one, feeding each back into the network, until the end mark or the length limit.
static void ResponderGenerate(Inference *inference, const Config *config, Random *random, char *output, int outputSize)
{
    int limit = config->maxAnswer < outputSize - 1 ? config->maxAnswer : outputSize - 1;
    int length = 0;

    while (length < limit)
    {
        int token = SamplerPick(inference->logits, VocabSize, config->temperature, config->topK, random);
        
        if (token == EndMark)
            break;

        output[length++] = (char)token;
        InferenceStep(inference, token);
    }

    output[length] = 0;
    Utf8Sanitize(output);
}

// Cleans the question the same way the training questions were cleaned. Returns a new string, or 0 when memory runs out.
static unsigned char *ResponderClean(const Config *config, const char *question, long *length)
{
    size_t size = strlen(question);
    unsigned char *clean = (unsigned char *)malloc(size + 1);

    if (clean == 0)
        return 0;

    if (config->normalize)
    {
        *length = NormalizeQuestion((const unsigned char *)question, (long)size, clean);
    }
    else
    {
        for (size_t i = 0; i < size; i++)
        {
            unsigned char byte = (unsigned char)question[i];
            clean[i] = byte < FirstTextByte ? ' ' : byte;
        }

        *length = (long)size;
    }

    clean[*length] = 0;
    return clean;
}

int ResponderAnswer(const Model *model, const Config *config, Random *random, ChatHistory *history, const char *question, char *output, int outputSize)
{
    long cleanLength = 0;
    unsigned char *clean = ResponderClean(config, question, &cleanLength);

    if (clean == 0)
        return 0;

    int useHistory = history != 0 && config->chatHistory;
    unsigned char *context = 0;
    long contextLength = 0;

    // A quarter of MaxLength stays free for the answer, because the model was trained on sequences of at most MaxLength bytes.
    long budget = (long)config->maxLength * 3 / 4 - cleanLength - 1;

    if (useHistory && budget > 0)
    {
        context = (unsigned char *)malloc((size_t)budget);
        
        if (context == 0)
        {
            free(clean);
            return 0;
        }

        contextLength = ChatHistoryContext(history, config->chatHistoryTurns, budget, context);
    }

    Inference inference;

    if (!InferenceCreate(&inference, model))
    {
        free(context);
        free(clean);

        return 0;
    }

    ResponderFeedBytes(&inference, context, contextLength);
    ResponderFeedBytes(&inference, clean, cleanLength);
    InferenceStep(&inference, Separator);
    ResponderGenerate(&inference, config, random, output, outputSize);
    InferenceFree(&inference);

    if (useHistory)
        ChatHistoryAdd(history, (const char *)clean, output);

    free(context);
    free(clean);
    return 1;
}

int ResponderContinue(const Model *model, const Config *config, Random *random, const char *text, char *output, int outputSize)
{
    Inference inference;

    if (!InferenceCreate(&inference, model))
        return 0;

    ResponderFeed(&inference, text);
    ResponderGenerate(&inference, config, random, output, outputSize);
    InferenceFree(&inference);
    
    return 1;
}
