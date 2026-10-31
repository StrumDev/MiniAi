#include "Augment.h"
#include <stdlib.h>
#include <string.h>

static int AugmentWordStart(const unsigned char *text, long index)
{
    return text[index] != ' ' && (index == 0 || text[index - 1] == ' ');
}

static long AugmentDropWord(Random *random, const unsigned char *text, long length, unsigned char *output)
{
    int words = 0;

    for (long i = 0; i < length; i++)
    {
        if (AugmentWordStart(text, i))
            words++;
    }

    if (words < 3)
        return 0;
    
    int dropped = (int)(RandomNext(random) % (unsigned int)words);
    int word = -1;
    long used = 0;

    for (long i = 0; i < length; i++)
    {
        if (AugmentWordStart(text, i))
            word++;
        
        if (word != dropped)
            output[used++] = text[i];
    }

    while (used > 0 && output[used - 1] == ' ')
        used--;

    return used;
}

// Removes one letter, or swaps two neighbouring letters. Works on whole UTF-8 characters.
static long AugmentLetters(Random *random, const unsigned char *text, long length, unsigned char *output, int swap)
{
    long *starts = (long *)malloc(((size_t)length + 1) * sizeof(long));
    
    if (starts == 0)
        return 0;

    long count = 0;
    
    for (long i = 0; i < length; i++)
    {
        if ((text[i] & 0xC0) != 0x80)
            starts[count++] = i;
    }

    starts[count] = length;

    long used = 0;

    if (count >= 4)
    {
        if (swap)
        {
            for (int attempt = 0; attempt < 8 && used == 0; attempt++)
            {
                long p = (long)(RandomNext(random) % (unsigned int)(count - 1));
                long firstLength = starts[p + 1] - starts[p];
                long secondLength = starts[p + 2] - starts[p + 1];
                int same = firstLength == secondLength && memcmp(text + starts[p], text + starts[p + 1], (size_t)firstLength) == 0;
                
                if (same || text[starts[p]] == ' ' || text[starts[p + 1]] == ' ')
                    continue;

                memcpy(output, text, (size_t)starts[p]);
                memcpy(output + starts[p], text + starts[p + 1], (size_t)secondLength);
                memcpy(output + starts[p] + secondLength, text + starts[p], (size_t)firstLength);
                memcpy(output + starts[p + 2], text + starts[p + 2], (size_t)(length - starts[p + 2]));
                
                used = length;
            }
        }
        else
        {
            long p = (long)(RandomNext(random) % (unsigned int)count);

            memcpy(output, text, (size_t)starts[p]);
            memcpy(output + starts[p], text + starts[p + 1], (size_t)(length - starts[p + 1]));
            
            used = length - (starts[p + 1] - starts[p]);
            
            while (used > 0 && output[used - 1] == ' ')
                used--;

            while (used > 0 && output[0] == ' ')
                memmove(output, output + 1, (size_t)(--used));
        }
    }

    free(starts);
    return used;
}

long AugmentQuestion(Random *random, const unsigned char *question, long length, unsigned char *output)
{
    unsigned int first = RandomNext(random) % 3;

    for (unsigned int attempt = 0; attempt < 3; attempt++)
    {
        unsigned int kind = (first + attempt) % 3;
        long result = kind == 0 ? AugmentDropWord(random, question, length, output) : AugmentLetters(random, question, length, output, kind == 1);
        
        if (result > 0)
            return result;
    }

    return 0;
}