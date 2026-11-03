#include "learning/BatchRunner.h"
#include "network/Backprop.h"

#ifdef _WIN32
#include <windows.h>
typedef HANDLE ThreadHandle;
#else
#include <pthread.h>
typedef pthread_t ThreadHandle;
#endif

enum
{
    MaxThreads = 256
};

typedef struct BatchJob
{
    const Model *model;
    Workspace *space;
    const Dataset *data;
    const int *indices;
    int count;
    int first;
    int stride;
    float questionWeight;
    int backward;
} BatchJob;

static void BatchWork(BatchJob *job)
{
    for (int i = job->first; i < job->count; i += job->stride)
    {
        int index = job->indices[i];

        BackpropSequence(job->model, job->space, DatasetTokens(job->data, index), DatasetLength(job->data, index),
            job->data->separators[index], job->questionWeight, job->backward);
    }
}

#ifdef _WIN32
static DWORD WINAPI BatchEntry(LPVOID argument)
{
    BatchWork((BatchJob *)argument);
    return 0;
}

static int BatchStart(ThreadHandle *handle, BatchJob *job)
{
    *handle = CreateThread(0, 0, BatchEntry, job, 0, 0);
    return *handle != 0;
}

static void BatchJoin(ThreadHandle handle)
{
    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
}
#else

static void *BatchEntry(void *argument)
{
    BatchWork((BatchJob *)argument);
    return 0;
}

static int BatchStart(ThreadHandle *handle, BatchJob *job)
{
    return pthread_create(handle, 0, BatchEntry, job) == 0;
}

static void BatchJoin(ThreadHandle handle)
{
    pthread_join(handle, 0);
}
#endif

BatchResult BatchRunnerRun(const Model *model, Workspace *spaces, int threads, const Dataset *data, const int *indices, int count, float questionWeight, float dropout, int backward)
{
    BatchResult result;
    result.loss = 0.0;
    result.weight = 0.0;
    result.tokens = 0;

    if (count <= 0)
        return result;

    int active = threads < count ? threads : count;

    if (active > MaxThreads)
        active = MaxThreads;

    BatchJob jobs[MaxThreads];
    ThreadHandle handles[MaxThreads];

    int started[MaxThreads];

    for (int t = 0; t < active; t++)
    {
        WorkspaceReset(&spaces[t], backward);
        spaces[t].dropout = dropout;
        jobs[t].model = model;
        jobs[t].space = &spaces[t];
        jobs[t].data = data;
        jobs[t].indices = indices;
        jobs[t].count = count;
        jobs[t].first = t;
        jobs[t].stride = active;
        jobs[t].questionWeight = questionWeight;
        jobs[t].backward = backward;
        started[t] = 0;
    }

    for (int t = 1; t < active; t++)
        started[t] = BatchStart(&handles[t], &jobs[t]);

    BatchWork(&jobs[0]);

    for (int t = 1; t < active; t++)
    {
        if (started[t])
            BatchJoin(handles[t]);
        else
            BatchWork(&jobs[t]);
    }

    for (int t = 0; t < active; t++)
    {
        result.loss += spaces[t].loss;
        result.weight += spaces[t].weight;
        result.tokens += spaces[t].tokens;

        if (backward && t > 0)
        {
            float *destination = spaces[0].gradients;
            const float *source = spaces[t].gradients;

            for (long i = 0; i < spaces[0].gradCount; i++)
                destination[i] += source[i];
        }
    }

    return result;
}