#include "Optimizer.h"
#include <math.h>
#include <stdlib.h>

int AdamCreate(Adam *adam, long count)
{
    adam->count = count;
    adam->step = 0;
    adam->first = (float *)calloc((size_t)count, sizeof(float));
    adam->second = (float *)calloc((size_t)count, sizeof(float));
    
    if (adam->first == 0 || adam->second == 0)
    {
        AdamFree(adam);
        return 0;
    }

    return 1;
}

void AdamFree(Adam *adam)
{
    free(adam->first);
    free(adam->second);

    adam->first = 0;
    adam->second = 0;
    adam->count = 0;
}

void AdamStep(Adam *adam, float *params, const float *gradients, float learningRate, float weightDecay)
{
    const float beta1 = 0.9f;
    const float beta2 = 0.999f;
    const float epsilon = 1e-8f;

    adam->step++;

    float correction1 = 1.0f - (float)pow((double)beta1, (double)adam->step);
    float correction2 = 1.0f - (float)pow((double)beta2, (double)adam->step);
    float stepSize = learningRate / correction1;
    float root2 = sqrtf(correction2);
    float decay = learningRate * weightDecay;
    float *first = adam->first;
    float *second = adam->second;

    for (long i = 0; i < adam->count; i++)
    {
        float gradient = gradients[i];
        float m = beta1 * first[i] + (1.0f - beta1) * gradient;
        float v = beta2 * second[i] + (1.0f - beta2) * gradient * gradient;

        first[i] = m;
        second[i] = v;

        float denominator = sqrtf(v) / root2 + epsilon;
        params[i] -= stepSize * m / denominator + decay * params[i];
    }
}

void OptimizerScale(float *gradients, long count, float factor)
{
    for (long i = 0; i < count; i++)
        gradients[i] *= factor;
}

float OptimizerClip(float *gradients, long count, float maxNorm)
{
    double sum = 0.0;

    for (long i = 0; i < count; i++)
        sum += (double)gradients[i] * (double)gradients[i];

    float norm = (float)sqrt(sum);

    if (maxNorm > 0.0f && isfinite(norm) && norm > maxNorm)
        OptimizerScale(gradients, count, maxNorm / norm);

    return norm;
}