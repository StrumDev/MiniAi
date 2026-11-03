#include "network/Workspace.h"
#include <stdlib.h>
#include <string.h>
#include "text/Tokens.h"

static float *WorkspaceArray(long count)
{
    return (float *)calloc((size_t)count, sizeof(float));
}

int WorkspaceCreate(Workspace *space, int layers, int hidden, int embedding, int maxLength)
{
    memset(space, 0, sizeof(Workspace));

    space->layers = layers;
    space->hidden = hidden;
    space->embedding = embedding;
    space->maxLength = maxLength;

    long slots = (long)layers * maxLength * hidden;

    space->update = WorkspaceArray(slots);
    space->reset = WorkspaceArray(slots);
    space->candidate = WorkspaceArray(slots);
    space->candidateHidden = WorkspaceArray(slots);
    space->state = WorkspaceArray(slots);
    space->drop = WorkspaceArray(slots);
    space->mask = WorkspaceArray(slots);
    space->gradTop = WorkspaceArray((long)maxLength * hidden);
    space->zeros = WorkspaceArray(hidden);
    space->forwardScratch = WorkspaceArray(6L * hidden);
    space->backwardScratch = WorkspaceArray(6L * hidden);
    space->carry = WorkspaceArray((long)layers * hidden);
    space->sum = WorkspaceArray(hidden);
    space->gradInput = WorkspaceArray(hidden > embedding ? hidden : embedding);
    space->probs = WorkspaceArray(VocabSize);
    space->gradCount = LayoutCount(layers, hidden, embedding);
    space->gradients = WorkspaceArray(space->gradCount);
    
    int ok = space->update && space->reset && space->candidate && space->candidateHidden && space->state && space->drop && space->mask;
    
    ok = ok && space->gradTop && space->zeros && space->forwardScratch && space->backwardScratch;
    ok = ok && space->carry && space->sum && space->gradInput && space->probs && space->gradients;
    
    if (!ok)
    {
        WorkspaceFree(space);
        return 0;
    }

    LayoutBind(&space->grads, space->gradients, layers, hidden, embedding);
    return 1;
}

void WorkspaceReset(Workspace *space, int clearGradients)
{
    space->loss = 0.0;
    space->weight = 0.0;
    space->tokens = 0;

    if (clearGradients)
        memset(space->gradients, 0, (size_t)space->gradCount * sizeof(float));
}

void WorkspaceFree(Workspace *space)
{
    free(space->update);
    free(space->reset);
    free(space->candidate);
    free(space->candidateHidden);
    free(space->state);
    free(space->drop);
    free(space->mask);
    free(space->gradTop);
    free(space->zeros);
    free(space->forwardScratch);
    free(space->backwardScratch);
    free(space->carry);
    free(space->sum);
    free(space->gradInput);
    free(space->probs);
    free(space->gradients);
    memset(space, 0, sizeof(Workspace));
}
