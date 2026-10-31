#include "Activation.h"

void ActivationSoftmax(float *values, int count)
{
    float maximum = values[0];

    for (int i = 1; i < count; i++)
    {
        if (values[i] > maximum)
            maximum = values[i];
    }

    float sum = 0.0f;

    for (int i = 0; i < count; i++)
    {
        values[i] = expf(values[i] - maximum);
        sum += values[i];
    }

    float inverse = 1.0f / sum;

    for (int i = 0; i < count; i++)
        values[i] *= inverse;
}