#include "learning/PairList.h"
#include <stdlib.h>
#include <string.h>

void PairListInit(PairList *list)
{
    memset(list, 0, sizeof(PairList));
}

static int PairListGrow(PairList *list)
{
    int capacity = list->capacity == 0 ? 256 : list->capacity * 2;
    unsigned char **questions = (unsigned char **)realloc(list->questions, (size_t)capacity * sizeof(unsigned char *));
    
    if (questions == 0)
        return 0;

    list->questions = questions;
    long *questionLengths = (long *)realloc(list->questionLengths, (size_t)capacity * sizeof(long));
    
    if (questionLengths == 0)
        return 0;

    list->questionLengths = questionLengths;
    unsigned char **answers = (unsigned char **)realloc(list->answers, (size_t)capacity * sizeof(unsigned char *));
    
    if (answers == 0)
        return 0;

    list->answers = answers;
    
    long *answerLengths = (long *)realloc(list->answerLengths, (size_t)capacity * sizeof(long));
    
    if (answerLengths == 0)
        return 0;

    list->answerLengths = answerLengths;
    list->capacity = capacity;
    return 1;
}

static unsigned char *PairListCopy(const unsigned char *bytes, long length)
{
    unsigned char *copy = (unsigned char *)malloc((size_t)length + 1);
    
    if (copy != 0)
    {
        memcpy(copy, bytes, (size_t)length);
        copy[length] = 0;
    }

    return copy;
}

int PairListAdd(PairList *list, const unsigned char *question, long questionLength, const unsigned char *answer, long answerLength)
{
    if (list->count == list->capacity && !PairListGrow(list))
        return 0;

    unsigned char *questionCopy = PairListCopy(question, questionLength);
    unsigned char *answerCopy = PairListCopy(answer, answerLength);
    
    if (questionCopy == 0 || answerCopy == 0)
    {
        free(questionCopy);
        free(answerCopy);
        return 0;
    }

    int index = list->count++;

    list->questions[index] = questionCopy;
    list->questionLengths[index] = questionLength;
    list->answers[index] = answerCopy;
    list->answerLengths[index] = answerLength;
    return 1;
}

void PairListFree(PairList *list)
{
    for (int i = 0; i < list->count; i++)
    {
        free(list->questions[i]);
        free(list->answers[i]);
    }

    free(list->questions);
    free(list->questionLengths);
    free(list->answers);
    free(list->answerLengths);
    
    memset(list, 0, sizeof(PairList));
}
