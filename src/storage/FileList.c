#include "storage/FileList.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

enum
{
    PathLimit = 2048
};

static int FileListEnds(const char *name, const char *extension)
{
    size_t nameLength = strlen(name);
    size_t extensionLength = strlen(extension);

    if (nameLength <= extensionLength)
        return 0;

    const char *tail = name + nameLength - extensionLength;
   
    for (size_t i = 0; i < extensionLength; i++)
    {
        if (tolower((unsigned char)tail[i]) != tolower((unsigned char)extension[i]))
            return 0;
    }

    return 1;
}

static int FileListCompare(const void *left, const void *right)
{
    return strcmp(*(const char *const *)left, *(const char *const *)right);
}

// Appends folder/name to the list. Returns nothing: a file that cannot be stored (no memory) is skipped.
static void FileListAdd(FileList *list, int *capacity, const char *folder, const char *name)
{
    size_t size = strlen(folder) + strlen(name) + 2;
    char *path = (char *)malloc(size);

    if (path == 0)
        return;

    snprintf(path, size, "%s/%s", folder, name);

    if (list->count == *capacity)
    {
        int grown = *capacity == 0 ? 16 : *capacity * 2;
        char **paths = (char **)realloc(list->paths, (size_t)grown * sizeof(char *));
        
        if (paths == 0)
        {
            free(path);
            return;
        }

        list->paths = paths;
        *capacity = grown;
    }

    list->paths[list->count++] = path;
}

#ifdef _WIN32
static int FileListCollect(FileList *list, const char *folder, const char *extension)
{
    char pattern[PathLimit];
    snprintf(pattern, sizeof(pattern), "%s/*", folder);
    
    WIN32_FIND_DATAA found;
    HANDLE search = FindFirstFileA(pattern, &found);

    if (search == INVALID_HANDLE_VALUE)
        return 0;

    int capacity = 0;
    do
    {
        int isFile = (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
        
        if (isFile && found.cFileName[0] != '.' && FileListEnds(found.cFileName, extension))
            FileListAdd(list, &capacity, folder, found.cFileName);
    }

    while (FindNextFileA(search, &found));
    
    FindClose(search);
    return 1;
}
#else

static int FileListCollect(FileList *list, const char *folder, const char *extension)
{
    DIR *directory = opendir(folder);

    if (directory == 0)
        return 0;

    int capacity = 0;
    struct dirent *entry;

    while ((entry = readdir(directory)) != 0)
    {
        if (entry->d_name[0] == '.' || !FileListEnds(entry->d_name, extension))
            continue;

        char path[PathLimit];
        snprintf(path, sizeof(path), "%s/%s", folder, entry->d_name);

        struct stat info;

        if (stat(path, &info) != 0 || !S_ISREG(info.st_mode))
            continue;

        FileListAdd(list, &capacity, folder, entry->d_name);
    }

    closedir(directory);
    return 1;
}
#endif

int FileListScan(FileList *list, const char *folder, const char *extension)
{
    list->count = 0;
    list->paths = 0;
    
    if (!FileListCollect(list, folder, extension))
        return 0;

    if (list->count > 1)
        qsort(list->paths, (size_t)list->count, sizeof(char *), FileListCompare);

    return 1;
}

void FileListFree(FileList *list)
{
    for (int i = 0; i < list->count; i++)
        free(list->paths[i]);

    free(list->paths);

    list->count = 0;
    list->paths = 0;
}