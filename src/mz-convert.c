/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>
#include <stdbool.h>
#include "mz-convert.h"

char *
mz_convert (const char *string,
            size_t length,
            const char *to_charset,
            const char *from_charset,
            size_t *bytes_read,
            size_t *bytes_written)
{
    iconv_t cd;
    char *dest = NULL;
    const char *src;
    char *outp;
    size_t src_left, dest_left;
    size_t ret;
    size_t outbuf_size;
    bool error = false;

    cd = iconv_open(to_charset, from_charset);

    src = string;

    outbuf_size = length + 1;
    dest_left = outbuf_size - 1;
    src_left = length;
    outp = dest = malloc(outbuf_size);

    do {
        ret = iconv(cd, (char**)&src, &src_left, &outp, &dest_left);
        if (ret == -1) {
            switch (errno) {
            case E2BIG:
                {
                    size_t used = outp - dest;

                    outbuf_size *= 2;
                    dest = realloc(dest, outbuf_size);

                    outp = dest + used;
                    dest_left = outbuf_size - used - 1;
                }
                break;
            case EINVAL:
            case EILSEQ:
            default:
                error = true;
                break;
            }
        }
    } while (ret == -1 && !error);

    iconv_close(cd);

    if (ret == -1) {
        free(dest);
        return NULL;
    }

    memset(outp, '\0', 1);

    return dest;
}

