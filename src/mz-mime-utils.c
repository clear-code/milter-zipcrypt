/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <stdlib.h>
#include <string.h>
#include "mz-mime-utils.h"
#include "mz-mime-private.h"

static unsigned char tohex[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

char *
mz_mime_utils_encode (const char *string)
{
    char *encoded;
    char *in = (char*)string;
    char *out;

    if (!string)
        return NULL;

    encoded = malloc(strlen(string) * 3);
    out = encoded;

    while (*in) {
        if (is_attrchar(*in)) {
            *out = '%';
            out++;
            *out = tohex[(*in >> 4) & 0xf];
            out++;
            *out = tohex[(*in) & 0xf];
        } else {
            *out = *in;
        }
        in++;
        out++;
    }
    *out = '\0';

    return encoded;
}

