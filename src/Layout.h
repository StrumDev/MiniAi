#pragma once

enum
{
    MaxLayers = 16
};

// Named views into one flat array of floats (weights, gradients, optimizer moments share this map).
typedef struct Layout
{
    float *embed;
    float *wx[MaxLayers];
    float *uh[MaxLayers];
    float *bx[MaxLayers];
    float *bh[MaxLayers];
    float *outWeight;
    float *outBias;
} Layout;

long LayoutCount(int layers, int hidden, int embedding);
void LayoutBind(Layout *layout, float *base, int layers, int hidden, int embedding);
