#pragma once

// Question/answer pairs read from .fin files, kept as plain bytes until the training data is built.
typedef struct PairList
{
    int count;
    int capacity;
    unsigned char **questions;
    long *questionLengths;
    unsigned char **answers;
    long *answerLengths;
} PairList;

void PairListInit(PairList *list);

// Copies both texts into the list. Returns 0 when memory runs out.
int PairListAdd(PairList *list, const unsigned char *question, long questionLength, const unsigned char *answer, long answerLength);

void PairListFree(PairList *list);
