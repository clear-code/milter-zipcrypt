/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
/* gbase64.c - Base64 encoding/decoding
 *
 *  Copyright (C) 2006 Alexander Larsson <alexl@redhat.com>
 *  Copyright (C) 2000-2003 Ximian Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * This is based on code in camel, written by:
 *    Michael Zucchi <notzed@ximian.com>
 *    Jeffrey Stedfast <fejj@ximian.com>
 * This file was picked from Glib-2.25.4.
 */

#include <string.h>
#include <limits.h>

#include "mz-base64.h"

static const char base64_alphabet[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned int
mz_base64_encode_step (const unsigned char *in,
                       unsigned int         len,
                       int                  break_lines,
                       char                *out,
                       int                 *state,
                       int                 *save)
{
    char *outptr;
    const unsigned char *inptr;

    if (!in)
        return 0;
    if (!out)
        return 0;
    if (!state)
        return 0;
    if (!save)
        return 0;
    if (len <= 0)
        return 0;

    inptr = in;
    outptr = out;

    if (len + ((char *) save) [0] > 2) {
        const unsigned char *inend = in+len-2;
        int c1, c2, c3;
        int already;

        already = *state;

        switch (((char *) save) [0]) {	
        case 1:	
            c1 = ((unsigned char *) save) [1]; 
            goto skip1;
        case 2:	
            c1 = ((unsigned char *) save) [1];
            c2 = ((unsigned char *) save) [2]; 
            goto skip2;
        }

        /* 
         * yes, we jump into the loop, no i'm not going to change it, 
         * it's beautiful! 
         */
        while (inptr < inend) {
            c1 = *inptr++;
skip1:
            c2 = *inptr++;
skip2:
            c3 = *inptr++;
            *outptr++ = base64_alphabet [ c1 >> 2 ];
            *outptr++ = base64_alphabet [ c2 >> 4 | 
                ((c1&0x3) << 4) ];
            *outptr++ = base64_alphabet [ ((c2 &0x0f) << 2) | 
                (c3 >> 6) ];
            *outptr++ = base64_alphabet [ c3 & 0x3f ];
            /* this is a bit ugly ... */
            if (break_lines && (++already) >= 19) {
                *outptr++ = '\n';
                already = 0;
            }
        }

        ((char *)save)[0] = 0;
        len = 2 - (inptr - inend);
        *state = already;
    }

    if (len>0) {
        char *saveout;

        /* points to the slot for the next char to save */
        saveout = & (((char *)save)[1]) + ((char *)save)[0];

        /* len can only be 0 1 or 2 */
        switch (len) {
        case 2:	*saveout++ = *inptr++;
        case 1:	*saveout++ = *inptr++;
        }
        ((char *)save)[0] += len;
    }

    return outptr - out;
}

unsigned int
mz_base64_encode_close (int   break_lines,
                        char *out,
                        int  *state,
                        int  *save)
{
    int c1, c2;
    char *outptr = out;

    if (!out)
        return 0;
    if (!state)
        return 0;
    if (!save)
        return 0;

    c1 = ((unsigned char *) save) [1];
    c2 = ((unsigned char *) save) [2];

    switch (((char *) save) [0]) {
    case 2:
        outptr[2] = base64_alphabet[ ( (c2 &0x0f) << 2 ) ];
        if (outptr[2] ==0)
            return 0;
        goto skip;
    case 1:
        outptr[2] = '=';
skip:
        outptr[0] = base64_alphabet [ c1 >> 2 ];
        outptr[1] = base64_alphabet [ c2 >> 4 | ( (c1&0x3) << 4 )];
        outptr[3] = '=';
        outptr += 4;
        break;
    }
    if (break_lines)
        *outptr++ = '\n';

    *save = 0;
    *state = 0;

    return outptr - out;
}

char *
mz_base64_encode (const unsigned char *data, 
                  unsigned int         len)
{
    char *out;
    int state = 0, outlen;
    int save = 0;

    if (!data)
        return NULL;

    if (len <= 0)
        return NULL;

    /* We can use a smaller limit here, since we know the saved state is 0,
       +1 is needed for trailing \0, also check for unlikely integer overflow */
    if (len >= ((UINT_MAX - 1) / 4 - 1) * 3)
        return NULL;

    out = malloc ((len / 3 + 1) * 4 + 1);

    outlen = mz_base64_encode_step (data, len, 0, out, &state, &save);
    outlen += mz_base64_encode_close (0, out + outlen, &state, &save);
    out[outlen] = '\0';

    return (char *) out;
}

static const unsigned char mime_base64_rank[256] = {
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,  0,255,255,
  255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255,
  255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};

unsigned int
mz_base64_decode_step (const char          *in,
                       unsigned int         len,
                       unsigned char       *out,
                       int                 *state,
                       unsigned int        *save)
{
    const unsigned char *inptr;
    unsigned char *outptr;
    const unsigned char *inend;
    unsigned char c, rank;
    unsigned char last[2];
    unsigned int v;
    int i;

    if (!in)
        return 0;
    if (!out)
        return 0;
    if (!state)
        return 0;
    if (!save)
        return 0;

    if (len <= 0)
        return 0;

    inend = (const unsigned char *)in+len;
    outptr = out;

    /* convert 4 base64 bytes to 3 normal bytes */
    v=*save;
    i=*state;
    inptr = (const unsigned char *)in;
    last[0] = last[1] = 0;
    while (inptr < inend) {
        c = *inptr++;
        rank = mime_base64_rank [c];
        if (rank != 0xff) {
            last[1] = last[0];
            last[0] = c;
            v = (v<<6) | rank;
            i++;
            if (i==4) {
                *outptr++ = v>>16;
                if (last[1] != '=')
                    *outptr++ = v>>8;
                if (last[0] != '=')
                    *outptr++ = v;
                i=0;
            }
        }
    }

    *save = v;
    *state = i;

    return outptr - out;
}

unsigned char *
mz_base64_decode (const char   *text,
                  unsigned int *out_len)
{
    unsigned char *ret;
    unsigned int input_length;
    int state = 0;
    unsigned int save = 0;

    if (!text)
        return NULL;

    if (!out_len)
        return NULL;

    input_length = strlen (text);

    if (input_length <= 1)
        return NULL;

    /* We can use a smaller limit here, since we know the saved state is 0,
       +1 used to avoid calling g_malloc0(0), and hence retruning NULL */
    ret = calloc (1, (input_length / 4) * 3 + 1);

    *out_len = mz_base64_decode_step (text, input_length, ret, &state, &save);

    return ret; 
}
 
unsigned char *
mz_base64_decode_inplace (char         *text,
                          unsigned int *out_len)
{
    int input_length, state = 0;
    unsigned int save = 0;

    if (!text)
        return NULL;

    if (!out_len)
        return NULL;

    input_length = strlen (text);

    if (input_length <= 1)
        return NULL;

    *out_len = mz_base64_decode_step (text, input_length, (unsigned char *) text, &state, &save);

    return (unsigned char *) text; 
}


