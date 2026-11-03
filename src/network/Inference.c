#include "network/Inference.h"
#include <stdlib.h>
#include <string.h>
#include "network/GruCell.h"
#include "network/Linear.h"

int InferenceCreate(Inference *inference, const Model *model)
{
    memset(inference, 0, sizeof(Inference));
    inference->model = model;
    
    int hidden = model->hidden;
    
    inference->state = (float *)calloc((size_t)model->layers * hidden, sizeof(float));
    inference->next = (float *)calloc((size_t)hidden, sizeof(float));
    inference->update = (float *)calloc((size_t)hidden, sizeof(float));
    inference->reset = (float *)calloc((size_t)hidden, sizeof(float));
    inference->candidate = (float *)calloc((size_t)hidden, sizeof(float));
    inference->candidateHidden = (float *)calloc((size_t)hidden, sizeof(float));
    inference->scratch = (float *)calloc((size_t)hidden * 6, sizeof(float));
    
    int ok = inference->state && inference->next && inference->update && inference->reset;
    ok = ok && inference->candidate && inference->candidateHidden && inference->scratch;
    
    if (!ok)
    {
        InferenceFree(inference);
        return 0;
    }

    return 1;
}

void InferenceStep(Inference *inference, int token)
{
    const Model *model = inference->model;
    const Layout *weights = &model->weights;
    
    int hidden = model->hidden;
    const float *input = weights->embed + (long)token * model->embedding;
    
    for (int layer = 0; layer < model->layers; layer++)
    {
        int inDim = layer == 0 ? model->embedding : hidden;
        float *state = inference->state + (long)layer * hidden;

        GruCellForward(weights->wx[layer], weights->uh[layer], weights->bx[layer], weights->bh[layer],
            inDim, hidden, input, state,
            inference->update, inference->reset, inference->candidate, inference->candidateHidden,
            inference->next, inference->scratch);
        
        memcpy(state, inference->next, (size_t)hidden * sizeof(float));
        input = state;
    }

    LinearMultiply(inference->logits, weights->outWeight, input, VocabSize, hidden, weights->outBias);
}

void InferenceFree(Inference *inference)
{
    free(inference->state);
    free(inference->next);
    free(inference->update);
    free(inference->reset);
    free(inference->candidate);
    free(inference->candidateHidden);
    free(inference->scratch);
    
    memset(inference, 0, sizeof(Inference));
}
