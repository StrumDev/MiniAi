#pragma once

typedef struct Random
{
    unsigned long long state;
} Random;

void RandomSeed(Random *random, unsigned long long seed);
unsigned int RandomNext(Random *random);
float RandomFloat(Random *random);
float RandomRange(Random *random, float low, float high);
void RandomShuffle(Random *random, int *items, int count);
