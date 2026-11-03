#pragma once

enum
{
    PathSize = 1024
};

// Looks for a file in the current folder, two levels up, and next to the program, each also under data/.
int LocatorFind(const char *fileName, const char *programPath, char *output, int outputSize);

// The same search for a folder (for example "training").
int LocatorFindFolder(const char *folderName, const char *programPath, char *output, int outputSize);

void LocatorDirectoryOf(const char *path, char *output, int outputSize);
