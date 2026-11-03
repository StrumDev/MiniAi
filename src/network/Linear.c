#include "network/Linear.h"

void LinearMultiply(float *restrict output, const float *restrict weights, const float *restrict input, int rows, int cols, const float *restrict bias)
{
    for (int i = 0; i < rows; i++)
    {
        const float *row = weights + (long)i * cols;
        
        float sum0 = 0.0f;
        float sum1 = 0.0f;
        float sum2 = 0.0f;
        float sum3 = 0.0f;
        float sum4 = 0.0f;
        float sum5 = 0.0f;
        float sum6 = 0.0f;
        float sum7 = 0.0f;
        int j = 0;
        
        for (; j + 8 <= cols; j += 8)
        {
            sum0 += row[j] * input[j];
            sum1 += row[j + 1] * input[j + 1];
            sum2 += row[j + 2] * input[j + 2];
            sum3 += row[j + 3] * input[j + 3];
            sum4 += row[j + 4] * input[j + 4];
            sum5 += row[j + 5] * input[j + 5];
            sum6 += row[j + 6] * input[j + 6];
            sum7 += row[j + 7] * input[j + 7];
        }
        
        float sum = ((sum0 + sum1) + (sum2 + sum3)) + ((sum4 + sum5) + (sum6 + sum7));
        
        for (; j < cols; j++)
            sum += row[j] * input[j];

        output[i] = bias != 0 ? sum + bias[i] : sum;
    }
}

void LinearTransposeAccumulate(float *restrict output, const float *restrict weights, const float *restrict input, int rows, int cols)
{
    for (int i = 0; i < rows; i++)
    {
        const float *row = weights + (long)i * cols;
        float scale = input[i];
        
        for (int j = 0; j < cols; j++)
            output[j] += row[j] * scale;
    }
}

void LinearOuterAccumulate(float *restrict matrix, const float *restrict left, const float *restrict right, int rows, int cols)
{
    for (int i = 0; i < rows; i++)
    {
        float *row = matrix + (long)i * cols;
        float scale = left[i];
        
        for (int j = 0; j < cols; j++)
            row[j] += scale * right[j];
    }
}