#include "Utf8.h"

void Utf8Sanitize(char *text)
{
    unsigned char *bytes = (unsigned char *)text;
    int i = 0;

    while (bytes[i] != 0)
    {
        int need = 0;

        if (bytes[i] < 0x80)
            need = 1;

        else if (bytes[i] >= 0xC2 && bytes[i] <= 0xDF)
            need = 2;

        else if (bytes[i] >= 0xE0 && bytes[i] <= 0xEF)
            need = 3;

        else if (bytes[i] >= 0xF0 && bytes[i] <= 0xF4)
            need = 4;

        int valid = need > 0;

        for (int k = 1; valid && k < need; k++)
        {
            if ((bytes[i + k] & 0xC0) != 0x80)
                valid = 0;
        }
        
        if (valid)
        {
            i += need;
        }
        else
        {
            bytes[i] = '?';
            i++;
        }
    }
}
