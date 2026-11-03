#include "storage/ModelStorage.h"
#include <stdio.h>
#include <string.h>
#include "storage/Locator.h"
#include "text/Tokens.h"

#ifdef _WIN32
#include <windows.h>
#endif

static const char Magic[8] = { 'M', 'I', 'N', 'I', 'A', 'I', '0', '1' };

// Moves the finished temporary file over the old model. On Windows rename() refuses to overwrite, so the system call is used.
static int ModelStorageReplace(const char *source, const char *target)
{
#ifdef _WIN32
    return MoveFileExA(source, target, MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return rename(source, target) == 0;
#endif
}

int ModelStorageSave(const Model *model, const char *path)
{
    char temporary[PathSize + 8];
    snprintf(temporary, sizeof(temporary), "%s.tmp", path);

    FILE *file = fopen(temporary, "wb");

    if (file == 0)
        return 0;

    int header[4] = { model->layers, model->hidden, model->embedding, VocabSize };
    long long count = model->count;
    
    int ok = fwrite(Magic, 1, sizeof(Magic), file) == sizeof(Magic);
    
    ok = ok && fwrite(header, sizeof(int), 4, file) == 4;
    ok = ok && fwrite(&count, sizeof(long long), 1, file) == 1;
    ok = ok && fwrite(model->params, sizeof(float), (size_t)model->count, file) == (size_t)model->count;
    
    if (fclose(file) != 0)
        ok = 0;

    if (!ok || !ModelStorageReplace(temporary, path))
    {
        remove(temporary);
        return 0;
    }

    return 1;
}

int ModelStorageLoad(Model *model, const char *path)
{
    FILE *file = fopen(path, "rb");
    
    if (file == 0)
        return 0;

    char magic[8];
    int header[4];
    long long count = 0;
    
    int ok = fread(magic, 1, sizeof(magic), file) == sizeof(magic);

    ok = ok && memcmp(magic, Magic, sizeof(Magic)) == 0;
    ok = ok && fread(header, sizeof(int), 4, file) == 4;
    ok = ok && fread(&count, sizeof(long long), 1, file) == 1;
    ok = ok && header[3] == VocabSize;
    ok = ok && header[0] >= 1 && header[0] <= MaxLayers;
    ok = ok && header[1] >= 1 && header[2] >= 1;
    ok = ok && count == LayoutCount(header[0], header[1], header[2]);
    
    if (!ok)
    {
        fclose(file);
        return 0;
    }

    if (!ModelCreate(model, header[0], header[1], header[2]))
    {
        fclose(file);
        return 0;
    }

    ok = fread(model->params, sizeof(float), (size_t)model->count, file) == (size_t)model->count;
    
    fclose(file);
    
    if (!ok)
    {
        ModelFree(model);
        return 0;
    }
    
    return 1;
}
