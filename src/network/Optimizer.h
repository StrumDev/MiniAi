#pragma once

typedef struct Adam
{
    long count;
    long step;
    float *first;
    float *second;
} Adam;

int AdamCreate(Adam *adam, long count);
void AdamFree(Adam *adam);
void AdamStep(Adam *adam, float *params, const float *gradients, float learningRate, float weightDecay);

void OptimizerScale(float *gradients, long count, float factor);

// Returns the gradient norm; shrinks the gradient to maxNorm when maxNorm is above zero.
float OptimizerClip(float *gradients, long count, float maxNorm);
