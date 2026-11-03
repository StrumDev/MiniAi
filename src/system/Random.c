#include "system/Random.h"

void RandomSeed(Random *random, unsigned long long seed)
{
    random->state = seed * 6364136223846793005ULL + 1442695040888963407ULL;

    if (random->state == 0)
        random->state = 88172645463325252ULL;

    RandomNext(random);
    RandomNext(random);
}

unsigned int RandomNext(Random *random)
{
    unsigned long long value = random->state;

    value ^= value >> 12;
    value ^= value << 25;
    value ^= value >> 27;

    random->state = value;

    return (unsigned int)((value * 2685821657736338717ULL) >> 32);
}

float RandomFloat(Random *random)
{
    return (float)(RandomNext(random) >> 8) * (1.0f / 16777216.0f);
}

float RandomRange(Random *random, float low, float high)
{
    return low + (high - low) * RandomFloat(random);
}

void RandomShuffle(Random *random, int *items, int count)
{
    for (int i = count - 1; i > 0; i--)
    {
        int j = (int)(RandomNext(random) % (unsigned int)(i + 1));
        
        int swap = items[i];
        items[i] = items[j];
        items[j] = swap;
    }
}
