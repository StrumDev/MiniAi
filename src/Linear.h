#pragma once

// output[i] = bias[i] + sum over j of weights[i * cols + j] * input[j]   (bias may be 0)
void LinearMultiply(float *output, const float *weights, const float *input, int rows, int cols, const float *bias);

// output[j] += sum over i of weights[i * cols + j] * input[i]
void LinearTransposeAccumulate(float *output, const float *weights, const float *input, int rows, int cols);

// matrix[i * cols + j] += left[i] * right[j]
void LinearOuterAccumulate(float *matrix, const float *left, const float *right, int rows, int cols);
