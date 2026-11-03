#include "network/Model.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "text/Tokens.h"

int ModelCreate(Model *model, int layers, int hidden, int embedding)
{
    memset(model, 0, sizeof(Model));

    model->layers = layers;
    model->hidden = hidden;
    model->embedding = embedding;
    model->count = LayoutCount(layers, hidden, embedding);
    model->params = (float *)calloc((size_t)model->count, sizeof(float));
    
    if (model->params == 0)
        return 0;

    LayoutBind(&model->weights, model->params, layers, hidden, embedding);
    return 1;
}

static void ModelFill(Random *random, float *target, long count, float bound)
{
    for (long i = 0; i < count; i++)
        target[i] = RandomRange(random, -bound, bound);
}

void ModelInitialize(Model *model, Random *random)
{
    memset(model->params, 0, (size_t)model->count * sizeof(float));

    int hidden = model->hidden;
    float bound = 1.0f / sqrtf((float)hidden);

    ModelFill(random, model->weights.embed, (long)VocabSize * model->embedding, 0.5f);

    for (int layer = 0; layer < model->layers; layer++)
    {
        long inDim = layer == 0 ? model->embedding : hidden;
        
        ModelFill(random, model->weights.wx[layer], 3L * hidden * inDim, bound);
        ModelFill(random, model->weights.uh[layer], 3L * hidden * hidden, bound);
    }

    ModelFill(random, model->weights.outWeight, (long)VocabSize * hidden, bound);
}

int ModelMatches(const Model *model, int layers, int hidden, int embedding)
{
    return model->layers == layers && model->hidden == hidden && model->embedding == embedding;
}

void ModelFree(Model *model)
{
    free(model->params);
    memset(model, 0, sizeof(Model));
}
