#include "Layout.h"
#include "Tokens.h"

long LayoutCount(int layers, int hidden, int embedding)
{
    long count = (long)VocabSize * embedding;

    for (int layer = 0; layer < layers; layer++)
    {
        long inDim = layer == 0 ? embedding : hidden;
        
        count += 3L * hidden * inDim;
        count += 3L * hidden * hidden;
        count += 6L * hidden;
    }

    count += (long)VocabSize * hidden;
    count += VocabSize;
    return count;
}

void LayoutBind(Layout *layout, float *base, int layers, int hidden, int embedding)
{
    float *cursor = base;
    layout->embed = cursor;
    cursor += (long)VocabSize * embedding;

    for (int layer = 0; layer < layers; layer++)
    {
        long inDim = layer == 0 ? embedding : hidden;

        layout->wx[layer] = cursor;
        cursor += 3L * hidden * inDim;

        layout->uh[layer] = cursor;
        cursor += 3L * hidden * hidden;

        layout->bx[layer] = cursor;
        cursor += 3L * hidden;

        layout->bh[layer] = cursor;
        cursor += 3L * hidden;
    }
    
    layout->outWeight = cursor;
    cursor += (long)VocabSize * hidden;
    layout->outBias = cursor;
}
