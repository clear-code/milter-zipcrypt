/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
/* gbase64.h - Base64 coding functions
 *
 *  Copyright (C) 2005  Alexander Larsson <alexl@redhat.com>
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
 */

#ifndef __MZ_BASE64_H__
#define __MZ_BASE64_H__

unsigned int  mz_base64_encode_step     (const unsigned char *in,
                                         unsigned int         len,
                                         int                  break_lines, /* boolean */
                                         char                *out,
                                         int                 *state,
                                         int                 *save);
unsigned int  mz_base64_encode_close    (int                 break_lines, /* boolean */
                                         char                *out,
                                         int                 *state,
                                         int                 *save);
char         *mz_base64_encode          (const unsigned char *data,
                                         unsigned int         len);
unsigned int  mz_base64_decode_step     (const char          *in,
                                         unsigned int         len,
                                         unsigned char       *out,
                                         int                 *state,
                                         unsigned int        *save);
unsigned char *mz_base64_decode         (const char          *text,
                                         unsigned int        *out_len);
unsigned char *mz_base64_decode_inplace (char                *text,
                                         unsigned int        *out_len);

#endif /* __MZ_BASE64_H__ */
