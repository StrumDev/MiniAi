#include "FinParser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Tokens.h"

enum
{
    KindBlank = 0,
    KindIn = 1,
    KindOut = 2,
    KindUnknown = 3,
    StateExpectIn = 0,
    StateHaveIn = 1,
    StateSkipOut = 2,
    MaxReported = 20
};

typedef struct FinLine
{
    int kind;
    const char *text;
    long length;
    const char *message;
} FinLine;

static int FinIsSpace(char value)
{
    return value == ' ' || value == '\t';
}

// Splits one line into its kind and quoted text. message stays 0 when the line is valid.
static void FinReadLine(const char *line, long length, FinLine *result)
{
    result->kind = KindBlank;
    result->text = 0;
    result->length = 0;
    result->message = 0;

    long start = 0;

    while (start < length && FinIsSpace(line[start]))
        start++;

    long end = length;
    
    while (end > start && (FinIsSpace(line[end - 1]) || line[end - 1] == '\r' || line[end - 1] == '\n'))
        end--;

    if (start == end || line[start] == '#')
        return;

    long position = start;
    
    if (end - start >= 3 && strncmp(line + start, "in:", 3) == 0)
    {
        result->kind = KindIn;
        position = start + 3;
    }
    else if (end - start >= 4 && strncmp(line + start, "out:", 4) == 0)
    {
        result->kind = KindOut;
        position = start + 4;
    }
    else
    {
        result->kind = KindUnknown;
        result->message = "the line must start with in: or out:";
        return;
    }

    while (position < end && FinIsSpace(line[position]))
        position++;

    if (position >= end || line[position] != '"')
    {
        result->message = "the opening quote is missing after the colon";
        return;
    }

    if (end - 1 <= position || line[end - 1] != '"')
    {
        result->message = "the closing quote is missing - the line must end with a quote (only spaces may follow it)";
        return;
    }

    result->text = line + position + 1;
    result->length = end - 1 - (position + 1);
    
    if (result->length == 0)
        result->message = "the text between the quotes is empty";
}

// Copies the text, replacing control characters (such as tabs) with spaces.
static unsigned char *FinCopyText(const char *text, long length)
{
    unsigned char *copy = (unsigned char *)malloc((size_t)length + 1);
    
    if (copy == 0)
        return 0;

    for (long i = 0; i < length; i++)
    {
        unsigned char byte = (unsigned char)text[i];
        copy[i] = byte < FirstTextByte ? ' ' : byte;
    }

    copy[length] = 0;
    return copy;
}

static void FinReport(const char *path, int lineNumber, const char *message, int *errors)
{
    (*errors)++;

    if (*errors <= MaxReported)
        printf("  %s:%d: %s\n", path, lineNumber, message);
}

// Reads one line (with its line break) into a growing buffer. Returns 0 at the end of the file or when memory runs out.
static int FinReadFileLine(char **line, size_t *capacity, FILE *file)
{
    size_t length = 0;
    int symbol;

    while ((symbol = fgetc(file)) != EOF)
    {
        if (length + 2 > *capacity)
        {
            size_t grown = *capacity == 0 ? 256 : *capacity * 2;
            char *buffer = (char *)realloc(*line, grown);
            
            if (buffer == 0)
                return 0;

            *line = buffer;
            *capacity = grown;
        }

        (*line)[length++] = (char)symbol;
        
        if (symbol == '\n')
            break;
    }
    
    if (length == 0)
        return 0;

    (*line)[length] = 0;
    return 1;
}

int FinParseFile(const char *path, PairList *list, int *added, int *errors)
{
    *added = 0;
    *errors = 0;

    FILE *file = fopen(path, "rb");

    if (file == 0)
        return 0;

    char *line = 0;
    size_t lineCapacity = 0;

    unsigned char *question = 0;
    long questionLength = 0;
    int questionLine = 0;
    int state = StateExpectIn;
    int lineNumber = 0;
    int memoryFailed = 0;

    while (!memoryFailed && FinReadFileLine(&line, &lineCapacity, file))
    {
        lineNumber++;
        const char *text = line;
        long length = (long)strlen(line);

        if (lineNumber == 1 && length >= 3 && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF)
        {
            text += 3;
            length -= 3;
        }

        FinLine parsed;
        FinReadLine(text, length, &parsed);

        if (parsed.kind == KindBlank)
            continue;

        if (parsed.kind == KindUnknown)
        {
            FinReport(path, lineNumber, parsed.message, errors);
            continue;
        }

        if (parsed.kind == KindIn)
        {
            if (state == StateHaveIn)
            {
                FinReport(path, questionLine, "in: has no out: after it", errors);
                free(question);
                question = 0;
            }

            if (parsed.message != 0)
            {
                FinReport(path, lineNumber, parsed.message, errors);
                state = StateSkipOut;
            }
            else
            {
                question = FinCopyText(parsed.text, parsed.length);
                questionLength = parsed.length;
                questionLine = lineNumber;
                memoryFailed = question == 0;
                state = StateHaveIn;
            }

            continue;
        }

        if (parsed.message != 0)
        {
            FinReport(path, lineNumber, parsed.message, errors);
            free(question);

            question = 0;
            state = StateExpectIn;
        }
        else if (state == StateHaveIn)
        {
            unsigned char *answer = FinCopyText(parsed.text, parsed.length);
            memoryFailed = answer == 0 || !PairListAdd(list, question, questionLength, answer, parsed.length);
            
            if (!memoryFailed)
                (*added)++;

            free(answer);
            free(question);

            question = 0;
            state = StateExpectIn;
        }
        else if (state == StateSkipOut)
        {
            state = StateExpectIn;
        }
        else
        {
            FinReport(path, lineNumber, "out: has no in: before it", errors);
        }
    }

    if (state == StateHaveIn && !memoryFailed)
        FinReport(path, questionLine, "in: has no out: after it", errors);

    if (memoryFailed)
        printf("  %s: not enough memory, the rest of the file was not read\n", path);

    if (*errors > MaxReported)
        printf("  %s: ... and %d more invalid entries\n", path, *errors - MaxReported);

    free(question);
    free(line);
    fclose(file);

    return 1;
}