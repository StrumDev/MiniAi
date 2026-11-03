#include "network/Backprop.h"
#include <math.h>
#include <string.h>
#include "network/Activation.h"
#include "network/GruCell.h"
#include "network/Linear.h"
#include "text/Tokens.h"

// What the next layer (or the output layer) sees from a layer: its state, or the state after dropout.
static const float *BackpropOutput(const Workspace *space, int layer, int step)
{
    return WorkspaceSlot(space, space->dropout > 0.0f ? space->drop : space->state, layer, step);
}

// Randomly silences part of a layer's output (inverted dropout: the survivors are scaled up).
static void BackpropDrop(Workspace *space, int layer, int step)
{
    float keep = 1.0f - space->dropout;
    float scale = 1.0f / keep;
    
    const float *state = WorkspaceSlot(space, space->state, layer, step);
    float *mask = WorkspaceSlot(space, space->mask, layer, step);
    float *drop = WorkspaceSlot(space, space->drop, layer, step);
    
    for (int j = 0; j < space->hidden; j++)
    {
        float value = RandomFloat(&space->random) < keep ? scale : 0.0f;
        mask[j] = value;
        drop[j] = state[j] * value;
    }
}

static const float *BackpropInput(const Model *model, const Workspace *space, const unsigned char *tokens, int layer, int step)
{
    if (layer == 0)
        return model->weights.embed + (long)tokens[step] * model->embedding;

    return BackpropOutput(space, layer - 1, step);
}

static const float *BackpropPrevious(const Workspace *space, int layer, int step)
{
    if (step == 0)
        return space->zeros;

    return WorkspaceSlot(space, space->state, layer, step - 1);
}

void BackpropSequence(const Model *model, Workspace *space, const unsigned char *tokens, int length, int separator, float questionWeight, int backward)
{
    const int layers = model->layers;
    const int hidden = model->hidden;
    const int embedding = model->embedding;
    const Layout *weights = &model->weights;

    if (length > space->maxLength)
        length = space->maxLength;

    int steps = length - 1;
    
    if (steps < 1)
        return;

    if (questionWeight <= 0.0f && separator >= steps)
        return;

    space->tokens += steps;

    for (int step = 0; step < steps; step++)
    {
        for (int layer = 0; layer < layers; layer++)
        {
            int inDim = layer == 0 ? embedding : hidden;
            
            GruCellForward(weights->wx[layer], weights->uh[layer], weights->bx[layer], weights->bh[layer],
                inDim, hidden, BackpropInput(model, space, tokens, layer, step), BackpropPrevious(space, layer, step),
                WorkspaceSlot(space, space->update, layer, step), WorkspaceSlot(space, space->reset, layer, step),
                WorkspaceSlot(space, space->candidate, layer, step), WorkspaceSlot(space, space->candidateHidden, layer, step),
                WorkspaceSlot(space, space->state, layer, step), space->forwardScratch);
            
            if (space->dropout > 0.0f)
                BackpropDrop(space, layer, step);
        }

        float weight = step + 1 > separator ? 1.0f : questionWeight;
        float *gradTop = space->gradTop + (long)step * hidden;
       
        if (weight <= 0.0f)
        {
            if (backward)
                memset(gradTop, 0, (size_t)hidden * sizeof(float));

            continue;
        }

        const float *top = BackpropOutput(space, layers - 1, step);
        float *probs = space->probs;
        
        LinearMultiply(probs, weights->outWeight, top, VocabSize, hidden, weights->outBias);
        ActivationSoftmax(probs, VocabSize);
        
        int target = tokens[step + 1];
        float probability = probs[target];
        space->loss -= (double)weight * log(probability > 1e-9f ? probability : 1e-9f);
        space->weight += weight;
        
        if (backward)
        {
            probs[target] -= 1.0f;
            
            for (int i = 0; i < VocabSize; i++)
                probs[i] *= weight;

            LinearOuterAccumulate(space->grads.outWeight, probs, top, VocabSize, hidden);
            
            for (int i = 0; i < VocabSize; i++)
                space->grads.outBias[i] += probs[i];

            memset(gradTop, 0, (size_t)hidden * sizeof(float));
            LinearTransposeAccumulate(gradTop, weights->outWeight, probs, VocabSize, hidden);
            
            if (space->dropout > 0.0f)
            {
                const float *topMask = WorkspaceSlot(space, space->mask, layers - 1, step);
                
                for (int j = 0; j < hidden; j++)
                    gradTop[j] *= topMask[j];
            }
        }
    }

    if (!backward)
        return;

    memset(space->carry, 0, (size_t)layers * hidden * sizeof(float));
    
    for (int step = steps - 1; step >= 0; step--)
    {
        const float *above = space->gradTop + (long)step * hidden;
        
        for (int layer = layers - 1; layer >= 0; layer--)
        {
            float *carry = space->carry + (long)layer * hidden;
            
            for (int j = 0; j < hidden; j++)
                space->sum[j] = above[j] + carry[j];

            int inDim = layer == 0 ? embedding : hidden;
            
            GruCellBackward(weights->wx[layer], weights->uh[layer],
                space->grads.wx[layer], space->grads.uh[layer], space->grads.bx[layer], space->grads.bh[layer],
                inDim, hidden, BackpropInput(model, space, tokens, layer, step), BackpropPrevious(space, layer, step),
                WorkspaceSlot(space, space->update, layer, step), WorkspaceSlot(space, space->reset, layer, step),
                WorkspaceSlot(space, space->candidate, layer, step), WorkspaceSlot(space, space->candidateHidden, layer, step),
                space->sum, space->gradInput, carry, space->backwardScratch);
            
            if (layer > 0 && space->dropout > 0.0f)
            {
                const float *lowerMask = WorkspaceSlot(space, space->mask, layer - 1, step);
                
                for (int j = 0; j < hidden; j++)
                    space->gradInput[j] *= lowerMask[j];
            }

            above = space->gradInput;
        }
        
        float *embedGrad = space->grads.embed + (long)tokens[step] * embedding;
        
        for (int j = 0; j < embedding; j++)
            embedGrad[j] += space->gradInput[j];
    }
}
