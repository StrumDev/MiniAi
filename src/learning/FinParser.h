#pragma once

#include "learning/PairList.h"

// Reads one .fin file and appends its question/answer pairs to the list.
//
//   in: "Hi"
//   out: "Hi! How can I help you today?"
//
// Spaces after the colon are ignored. The text is everything between the opening quote and the LAST quote of the line,
// so quotes inside the text are ordinary characters. The closing quote must be the last character of the line;
// only spaces and tabs may follow it. Empty lines and lines starting with # are ignored.
// Invalid entries are reported with file name and line number and skipped.
//
// Returns 0 when the file cannot be opened. added receives the number of pairs, errors the number of invalid entries.
int FinParseFile(const char *path, PairList *list, int *added, int *errors);
