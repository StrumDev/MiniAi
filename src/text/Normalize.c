#include "text/Normalize.h"
#include <string.h>
#include "text/Tokens.h"

static int NormalizeIsDigit(unsigned char byte)
{
    return byte >= '0' && byte <= '9';
}

// Returns how many bytes of a separator start at index (0 when the byte is an ordinary character).
static int NormalizeBreakLength(const unsigned char *source, long length, long index)
{
    unsigned char byte = source[index];

    if (byte < FirstTextByte || byte == ' ')
        return 1;
    
    if (strchr("?!\"();:,", byte) != 0)
        return 1;

    if (byte == '.')
    {
        int inNumber = index > 0 && index + 1 < length && NormalizeIsDigit(source[index - 1]) && NormalizeIsDigit(source[index + 1]);
        return inNumber ? 0 : 1;
    }

    if (byte == 0xC2 && index + 1 < length && (source[index + 1] == 0xAB || source[index + 1] == 0xBB))
        return 2;

    return 0;
}

// Lowercases a two-byte Cyrillic letter in place. Returns 1 when the two bytes were a Cyrillic letter.
static int NormalizeCyrillic(unsigned char *pair)
{
    unsigned int point = ((unsigned int)(pair[0] & 0x1F) << 6) | (unsigned int)(pair[1] & 0x3F);
    
    if (point >= 0x410 && point <= 0x42F)
        point += 0x20;
    
    else if (point >= 0x400 && point <= 0x40F)
        point += 0x50;
    
    else if (point == 0x490)
        point = 0x491;

    pair[0] = (unsigned char)(0xC0 | (point >> 6));
    pair[1] = (unsigned char)(0x80 | (point & 0x3F));
    return 1;
}

long NormalizeQuestion(const unsigned char *source, long length, unsigned char *output)
{
    long used = 0;
    int pending = 0;
    long i = 0;

    while (i < length)
    {
        int skip = NormalizeBreakLength(source, length, i);

        if (skip > 0)
        {
            pending = 1;
            i += skip;
            
            continue;
        }

        if (pending && used > 0)
            output[used++] = ' ';

        pending = 0;
        unsigned char byte = source[i];

        if (byte >= 'A' && byte <= 'Z')
        {
            output[used++] = (unsigned char)(byte + ('a' - 'A'));
            i++;
        }
        else if (byte >= 0xD0 && byte <= 0xD2 && i + 1 < length && (source[i + 1] & 0xC0) == 0x80)
        {
            output[used] = byte;
            output[used + 1] = source[i + 1];
            
            NormalizeCyrillic(output + used);
            
            used += 2;
            i += 2;
        }
        else
        {
            output[used++] = byte;
            i++;
        }
    }

    return used;
}