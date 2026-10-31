#pragma once

typedef struct FileList
{
    int count;
    char **paths;
} FileList;

// Collects the regular files of a folder whose names end with the extension (for example ".fin"), sorted by name.
// Returns 0 when the folder cannot be read.
int FileListScan(FileList *list, const char *folder, const char *extension);

void FileListFree(FileList *list);
