#include "Sampler.h"
#include <math.h>
#include "Tokens.h"

int SamplerPick(const float *logits, int count, float temperature, int topK, Random *random)
{
    float scores[VocabSize];
    int tokens[VocabSize];
    int available = 0;

    for (int i = 0; i < count && i < VocabSize; i++)
    {
        if (i >= FirstTextByte || i == EndMark)
        {
            scores[available] = logits[i];
            tokens[available] = i;
            
            available++;
        }
    }

    int keep = available;

    if (temperature <= 0.01f)
        keep = 1;
    
    else if (topK > 0 && topK < available)
        keep = topK;

    if (keep < available)
    {
        for (int k = 0; k < keep; k++)
        {
            int best = k;

            for (int j = k + 1; j < available; j++)
            {
                if (scores[j] > scores[best])
                    best = j;
            }

            float scoreSwap = scores[k];
            scores[k] = scores[best];
            scores[best] = scoreSwap;

            int tokenSwap = tokens[k];
            tokens[k] = tokens[best];
            tokens[best] = tokenSwap;
        }
    }

    if (keep == 1)
        return tokens[0];

    float maximum = scores[0];

    for (int k = 1; k < keep; k++)
    {
        if (scores[k] > maximum)
            maximum = scores[k];
    }

    float sum = 0.0f;

    for (int k = 0; k < keep; k++)
    {
        scores[k] = expf((scores[k] - maximum) / temperature);
        sum += scores[k];
    }

    float pick = RandomFloat(random) * sum;

    for (int k = 0; k < keep; k++)
    {
        pick -= scores[k];

        if (pick <= 0.0f)
            return tokens[k];
    }

    return tokens[keep - 1];
}