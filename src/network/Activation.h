#pragma once

#include <math.h>

static inline float ActivationSigmoid(float value)
{
    return 1.0f / (1.0f + expf(-value));
}

static inline float ActivationTanh(float value)
{
    return tanhf(value);
}

void ActivationSoftmax(float *values, int count);
