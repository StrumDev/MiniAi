#pragma once

// Gate layout inside every 3 * hidden vector: [update | reset | candidate].
// scratch must hold 6 * hidden floats.

void GruCellForward(const float *wx, const float *uh, const float *bx, const float *bh,
    int inDim, int hidden, const float *input, const float *previous,
    float *update, float *reset, float *candidate, float *candidateHidden,
    float *output, float *scratch);

void GruCellBackward(const float *wx, const float *uh,
    float *gradWx, float *gradUh, float *gradBx, float *gradBh,
    int inDim, int hidden, const float *input, const float *previous,
    const float *update, const float *reset, const float *candidate, const float *candidateHidden,
    const float *gradOutput, float *gradInput, float *gradPrevious, float *scratch);
