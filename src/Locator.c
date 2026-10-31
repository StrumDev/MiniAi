#include "Locator.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

enum
{
    BaseCount = 6
};

static int LocatorReadable(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file == 0)
        return 0;

    fclose(file);
    return 1;
}

static int LocatorIsFolder(const char *path)
{
#ifdef _WIN32
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat info;
    return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
#endif
}

// The last folder separator of a path. Windows paths may use both kinds.
static const char *LocatorLastSeparator(const char *path)
{
    const char *slash = strrchr(path, '/');
#ifdef _WIN32
    const char *backslash = strrchr(path, '\\');
    
    if (backslash != 0 && (slash == 0 || backslash > slash))
        slash = backslash;
#endif
    return slash;
}

void LocatorDirectoryOf(const char *path, char *output, int outputSize)
{
    const char *slash = LocatorLastSeparator(path);
    if (slash == 0)
    {
        snprintf(output, outputSize, ".");
        return;
    }

    if (slash == path)
    {
        snprintf(output, outputSize, "/");
        return;
    }

    int length = (int)(slash - path);

    if (length >= outputSize)
        length = outputSize - 1;

    memcpy(output, path, (size_t)length);
    output[length] = 0;
}

static void LocatorProgramDirectory(const char *programPath, char *output, int outputSize)
{
    char buffer[PathSize];
#ifdef _WIN32
    long length = (long)GetModuleFileNameA(0, buffer, (DWORD)(sizeof(buffer) - 1));
    
    if (length >= (long)(sizeof(buffer) - 1))
        length = 0;
#else
    long length = (long)readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
#endif
    if (length > 0)
    {
        buffer[length] = 0;
        LocatorDirectoryOf(buffer, output, outputSize);
        return;
    }

    if (programPath != 0 && programPath[0] != 0)
    {
        LocatorDirectoryOf(programPath, output, outputSize);
        return;
    }

    snprintf(output, outputSize, ".");
}

static void LocatorBases(const char *programPath, char bases[BaseCount][PathSize])
{
    char programDirectory[PathSize];

    LocatorProgramDirectory(programPath, programDirectory, PathSize);

    snprintf(bases[0], PathSize, ".");
    snprintf(bases[1], PathSize, "..");
    snprintf(bases[2], PathSize, "../..");
    snprintf(bases[3], PathSize, "%s", programDirectory);
    snprintf(bases[4], PathSize, "%s/..", programDirectory);
    snprintf(bases[5], PathSize, "%s/../..", programDirectory);
}

static int LocatorSearch(const char *name, const char *programPath, char *output, int outputSize, int folder)
{
    char bases[BaseCount][PathSize];

    LocatorBases(programPath, bases);

    const char *prefixes[2] = { "", "data/" };

    for (int b = 0; b < BaseCount; b++)
    {
        for (int f = 0; f < 2; f++)
        {
            char candidate[PathSize * 2];
            snprintf(candidate, sizeof(candidate), "%s/%s%s", bases[b], prefixes[f], name);
            
            if (folder ? LocatorIsFolder(candidate) : LocatorReadable(candidate))
            {
                snprintf(output, outputSize, "%s", candidate);
                return 1;
            }
        }
    }
    
    output[0] = 0;
    return 0;
}

int LocatorFind(const char *fileName, const char *programPath, char *output, int outputSize)
{
    return LocatorSearch(fileName, programPath, output, outputSize, 0);
}

int LocatorFindFolder(const char *folderName, const char *programPath, char *output, int outputSize)
{
    return LocatorSearch(folderName, programPath, output, outputSize, 1);
}
