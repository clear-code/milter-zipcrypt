/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
/*
 *  Copyright (C) 2000-2009 Jeffrey Stedfast
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

#ifndef __MZ_QUOTED_PRINTABLE_H__
#define __MZ_QUOTED_PRINTABLE_H__

#define MZ_QP_ENCODE_LEN(x)     ((size_t) ((((x) / 24) * 74) + 74))

unsigned int mz_quoted_printable_encode_step  (const unsigned char *inbuf,
                                               unsigned int         inlen,
                                               unsigned char       *outbuf,
                                               int                 *state,
                                               unsigned int        *save);
unsigned int mz_quoted_printable_encode_close (const unsigned char *inbuf,
                                               unsigned int         inlen,
                                               unsigned char       *outbuf,
                                               int                 *state,
                                               unsigned int        *save);

#endif /* __MZ_QUOTED_PRINTABLE_H__ */
