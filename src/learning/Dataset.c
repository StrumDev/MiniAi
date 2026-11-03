#include "learning/Dataset.h"
#include <stdlib.h>
#include <string.h>

int DatasetInit(Dataset *data)
{
    memset(data, 0, sizeof(Dataset));

    data->countCapacity = 1024;
    data->byteCapacity = 1L << 20;
    data->offsets = (long *)malloc(((size_t)data->countCapacity + 1) * sizeof(long));
    data->separators = (int *)malloc((size_t)data->countCapacity * sizeof(int));
    data->origins = (int *)malloc((size_t)data->countCapacity * sizeof(int));
    data->bytes = (unsigned char *)malloc((size_t)data->byteCapacity);
    
    if (data->offsets == 0 || data->separators == 0 || data->origins == 0 || data->bytes == 0)
    {
        DatasetFree(data);
        return 0;
    }

    data->offsets[0] = 0;
    return 1;
}

static int DatasetReserve(Dataset *data, long extraBytes)
{
    if (data->count + 1 >= data->countCapacity)
    {
        int capacity = data->countCapacity * 2;
        long *offsets = (long *)realloc(data->offsets, ((size_t)capacity + 1) * sizeof(long));

        if (offsets == 0)
            return 0;

        data->offsets = offsets;
        int *separators = (int *)realloc(data->separators, (size_t)capacity * sizeof(int));

        if (separators == 0)
            return 0;

        data->separators = separators;
        int *origins = (int *)realloc(data->origins, (size_t)capacity * sizeof(int));

        if (origins == 0)
            return 0;

        data->origins = origins;
        data->countCapacity = capacity;
    }

    long needed = data->offsets[data->count] + extraBytes;

    if (needed > data->byteCapacity)
    {
        long capacity = data->byteCapacity;

        while (capacity < needed)
            capacity *= 2;

        unsigned char *bytes = (unsigned char *)realloc(data->bytes, (size_t)capacity);

        if (bytes == 0)
            return 0;

        data->bytes = bytes;
        data->byteCapacity = capacity;
    }

    return 1;
}

int DatasetAdd(Dataset *data, const unsigned char *bytes, long length, int separator, int origin)
{
    if (!DatasetReserve(data, length))
        return 0;

    int index = data->count;

    memcpy(data->bytes + data->offsets[index], bytes, (size_t)length);
    
    data->separators[index] = separator;
    data->origins[index] = origin < 0 ? index : origin;
    data->offsets[index + 1] = data->offsets[index] + length;
    data->count++;

    return 1;
}

int DatasetMerge(Dataset *target, const Dataset *source)
{
    int base = target->count;

    for (int i = 0; i < source->count; i++)
    {
        if (!DatasetAdd(target, DatasetTokens(source, i), DatasetLength(source, i), source->separators[i], base + source->origins[i]))
            return 0;
    }

    return 1;
}

void DatasetFree(Dataset *data)
{
    free(data->bytes);
    free(data->offsets);
    free(data->separators);
    free(data->origins);

    memset(data, 0, sizeof(Dataset));
}