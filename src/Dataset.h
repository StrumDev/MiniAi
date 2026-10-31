#pragma once

// A list of training sequences stored as raw bytes in one block.
// Question/answer examples: question, Separator, answer, EndMark (separator = question length).
// Plain text windows: just the text (separator = -1, so every position is learned).
// origins[i] is the example that example i was made from: itself, or the original of an augmented copy.
typedef struct Dataset
{
    int count;
    int countCapacity;
    long byteCapacity;
    unsigned char *bytes;
    long *offsets;
    int *separators;
    int *origins;
} Dataset;

int DatasetInit(Dataset *data);

// origin below zero means "this example is its own original".
int DatasetAdd(Dataset *data, const unsigned char *bytes, long length, int separator, int origin);

// Appends every example of source to target.
int DatasetMerge(Dataset *target, const Dataset *source);

void DatasetFree(Dataset *data);

static inline int DatasetLength(const Dataset *data, int index)
{
    return (int)(data->offsets[index + 1] - data->offsets[index]);
}

static inline const unsigned char *DatasetTokens(const Dataset *data, int index)
{
    return data->bytes + data->offsets[index];
}

static inline int DatasetOriginals(const Dataset *data)
{
    int originals = 0;

    for (int i = 0; i < data->count; i++)
    {
        if (data->origins[i] == i)
            originals++;
    }
    
    return originals;
}
