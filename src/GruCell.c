#include "GruCell.h"
#include "Activation.h"
#include "Linear.h"

void GruCellForward(const float *wx, const float *uh, const float *bx, const float *bh,
    int inDim, int hidden, const float *input, const float *previous,
    float *update, float *reset, float *candidate, float *candidateHidden,
    float *output, float *scratch)
{
    float *gateInput = scratch;
    float *gateHidden = scratch + 3 * hidden;
    
    LinearMultiply(gateInput, wx, input, 3 * hidden, inDim, bx);
    LinearMultiply(gateHidden, uh, previous, 3 * hidden, hidden, bh);
    
    for (int j = 0; j < hidden; j++)
    {
        float z = ActivationSigmoid(gateInput[j] + gateHidden[j]);
        float r = ActivationSigmoid(gateInput[hidden + j] + gateHidden[hidden + j]);
        float hiddenPart = gateHidden[2 * hidden + j];
        float n = ActivationTanh(gateInput[2 * hidden + j] + r * hiddenPart);
        
        update[j] = z;
        reset[j] = r;
        candidate[j] = n;
        candidateHidden[j] = hiddenPart;
        output[j] = (1.0f - z) * n + z * previous[j];
    }
}

void GruCellBackward(const float *wx, const float *uh,
    float *gradWx, float *gradUh, float *gradBx, float *gradBh,
    int inDim, int hidden, const float *input, const float *previous,
    const float *update, const float *reset, const float *candidate, const float *candidateHidden,
    const float *gradOutput, float *gradInput, float *gradPrevious, float *scratch)
{
    float *gateInput = scratch;
    float *gateHidden = scratch + 3 * hidden;
    
    for (int j = 0; j < hidden; j++)
    {
        float z = update[j];
        float r = reset[j];
        float n = candidate[j];
        float dh = gradOutput[j];
        float dCandidate = dh * (1.0f - z);
        float dUpdate = dh * (previous[j] - n);
        float dCandidatePre = dCandidate * (1.0f - n * n);
        float dUpdatePre = dUpdate * z * (1.0f - z);
        float dReset = dCandidatePre * candidateHidden[j];
        float dResetPre = dReset * r * (1.0f - r);
        
        gateInput[j] = dUpdatePre;
        gateInput[hidden + j] = dResetPre;
        gateInput[2 * hidden + j] = dCandidatePre;
        gateHidden[j] = dUpdatePre;
        gateHidden[hidden + j] = dResetPre;
        gateHidden[2 * hidden + j] = dCandidatePre * r;
        gradPrevious[j] = dh * z;
    }
    
    LinearOuterAccumulate(gradWx, gateInput, input, 3 * hidden, inDim);
    LinearOuterAccumulate(gradUh, gateHidden, previous, 3 * hidden, hidden);
    
    for (int i = 0; i < 3 * hidden; i++)
    {
        gradBx[i] += gateInput[i];
        gradBh[i] += gateHidden[i];
    }

    for (int i = 0; i < inDim; i++)
        gradInput[i] = 0.0f;

    LinearTransposeAccumulate(gradInput, wx, gateInput, 3 * hidden, inDim);
    LinearTransposeAccumulate(gradPrevious, uh, gateHidden, 3 * hidden, hidden);
}