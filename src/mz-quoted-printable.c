/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
/*
 *  Copyright (C) 2000-2009 Jeffrey Stedfast and Michael Zucchi
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#include "mz-quoted-printable.h"
#include "mz-mime-private.h"

static unsigned char tohex[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

unsigned int
mz_quoted_printable_encode_close (const unsigned char *inbuf,
                                  unsigned int inlen,
                                  unsigned char *outbuf,
                                  int *state,
                                  unsigned int *save)
{
    register unsigned char *outptr = outbuf;
    int last;

    if (inlen > 0)
        outptr += mz_quoted_printable_encode_step(inbuf, inlen, outptr, state, save);

    last = *state;
    if (last != -1) {
        /* space/tab must be encoded if its the last character on
           the line */
        if (is_qpsafe(last) && !is_blank(last)) {
            *outptr++ = last;
        } else {
            *outptr++ = '=';
            *outptr++ = tohex[(last >> 4) & 0xf];
            *outptr++ = tohex[last & 0xf];
        }
    }

    *outptr++ = '\n';

    *save = 0;
    *state = -1;

    return (outptr - outbuf);
}

unsigned int
mz_quoted_printable_encode_step (const unsigned char *inbuf,
                                 unsigned int inlen,
                                 unsigned char *outbuf,
                                 int *state,
                                 unsigned int *save)
{
	const register unsigned char *inptr = inbuf;
	const unsigned char *inend = inbuf + inlen;
	register unsigned char *outptr = outbuf;
	register unsigned int sofar = *save;  /* keeps track of how many chars on a line */
	register int last = *state;  /* keeps track if last char to end was a space cr etc */
	unsigned char c;
	
    while (inptr < inend) {
        c = *inptr++;
        if (c == '\r') {
            if (last != -1) {
                *outptr++ = '=';
                *outptr++ = tohex[(last >> 4) & 0xf];
                *outptr++ = tohex[last & 0xf];
                sofar += 3;
            }
            last = c;
        } else if (c == '\n') {
            if (last != -1 && last != '\r') {
                *outptr++ = '=';
                *outptr++ = tohex[(last >> 4) & 0xf];
                *outptr++ = tohex[last & 0xf];
            }
            *outptr++ = '\n';
            sofar = 0;
            last = -1;
        } else {
            if (last != -1) {
                if (is_qpsafe(last)) {
                    *outptr++ = last;
                    sofar++;
                } else {
                    *outptr++ = '=';
                    *outptr++ = tohex[(last >> 4) & 0xf];
                    *outptr++ = tohex[last & 0xf];
                    sofar += 3;
                }
            }

            if (is_qpsafe(c)) {
                if (sofar > 74) {
                    *outptr++ = '=';
                    *outptr++ = '\n';
                    sofar = 0;
                }

                /* delay output of space char */
                if (is_blank(c)) {
                    last = c;
                } else {
                    *outptr++ = c;
                    sofar++;
                    last = -1;
                }
            } else {
                if (sofar > 72) {
                    *outptr++ = '=';
                    *outptr++ = '\n';
                    sofar = 3;
                } else
                    sofar += 3;

                *outptr++ = '=';
                *outptr++ = tohex[(c >> 4) & 0xf];
                *outptr++ = tohex[c & 0xf];
                last = -1;
            }
        }
    }

    *save = sofar;
    *state = last;

    return (outptr - outbuf);
}

