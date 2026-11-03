#pragma once

#include "learning/Dataset.h"

// Pretraining data. Reads one plain text file (.pre, any layout), turns line breaks and runs of spaces into single spaces,
// cuts the text into windows of `window` bytes that start every `stride` bytes (0 = same as window)
// and adds the windows to an already initialized dataset. Windows never cross from one file into the next.
// Every position of a window is learned: the target is simply the next byte of the text.
// Returns 0 when the file cannot be read or memory runs out. textBytes receives the size of the cleaned text.
int DatasetAddText(Dataset *data, const char *path, int window, int stride, long *textBytes);
