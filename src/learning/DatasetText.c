#include "learning/DatasetText.h"
#include <stdio.h>
#include <stdlib.h>
#include "text/Tokens.h"

// Reads the whole file and writes the cleaned text into the returned buffer.
static unsigned char *DatasetTextRead(const char *path, long *length)
{
    FILE *file = fopen(path, "rb");

    if (file == 0)
        return 0;

    fseek(file, 0, SEEK_END);

    long size = ftell(file);

    fseek(file, 0, SEEK_SET);

    if (size < 0)
    {
        fclose(file);
        return 0;
    }

    unsigned char *text = (unsigned char *)malloc((size_t)size + 1);

    if (text == 0)
    {
        fclose(file);
        return 0;
    }

    long read = (long)fread(text, 1, (size_t)size, file);
    fclose(file);

    long start = 0;

    if (read >= 3 && text[0] == 0xEF && text[1] == 0xBB && text[2] == 0xBF)
        start = 3;

    long used = 0;
    int space = 1;

    for (long i = start; i < read; i++)
    {
        unsigned char byte = text[i];

        if (byte < FirstTextByte)
            byte = ' ';

        if (byte == ' ')
        {
            if (space)
                continue;

            space = 1;
        }
        else
            space = 0;

        text[used++] = byte;
    }
   
    if (used > 0 && text[used - 1] == ' ')
        used--;

    *length = used;
    return text;
}

int DatasetAddText(Dataset *data, const char *path, int window, int stride, long *textBytes)
{
    long length = 0;
    *textBytes = 0;

    unsigned char *text = DatasetTextRead(path, &length);
    if (text == 0)
        return 0;

    *textBytes = length;
    
    if (stride <= 0 || stride > window)
        stride = window;

    int ok = 1;
    long position = 0;

    while (ok && position + 1 < length)
    {
        // Never start a window in the middle of a UTF-8 character.
        while (position < length && (text[position] & 0xC0) == 0x80)
            position++;

        long size = length - position < window ? length - position : window;
        
        if (size < 2)
            break;

        ok = DatasetAdd(data, text + position, size, -1, -1);
        
        if (position + size >= length)
            break;

        position += stride;
    }

    free(text);
    return ok;
}