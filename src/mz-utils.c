/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <iconv.h>
#include "mz-utils.h"
#include "mz-attachment.h"
#include "base64.h"

#define CONTENT_TYPE_STRING "Content-type:"

char *
mz_utils_get_content_type (const char *contents)
{
    const char *line_feed;
    const char *line = contents;

    while ((line_feed = strchr(line, '\n'))) {
        if (!strncasecmp(CONTENT_TYPE_STRING, line, strlen(CONTENT_TYPE_STRING))) {
            const char *content_type;
            char *semicolon;
            content_type = line + strlen(CONTENT_TYPE_STRING);
            while (*content_type == ' ') {
                content_type++;
            }
            semicolon = strchr(content_type, ';');

            if (!semicolon)
                return strndup(content_type, line_feed - content_type);
            return strndup(content_type, semicolon - content_type);
        }
        line = line_feed + 1;
    }
    return NULL;
}

#define CONTENT_TRANSFER_ENCODING_STRING "Content-Transfer-Encoding:"
char *
mz_utils_get_content_transfer_encoding (const char *contents)
{
    const char *line_feed;
    const char *line = contents;

    while ((line_feed = strchr(line, '\n'))) {
        if (!strncasecmp(CONTENT_TRANSFER_ENCODING_STRING,
                         line,
                         strlen(CONTENT_TRANSFER_ENCODING_STRING))) {
            const char *content_type;
            content_type = line + strlen(CONTENT_TRANSFER_ENCODING_STRING);
            while (*content_type == ' ') {
                content_type++;
            }

            return strndup(content_type, line_feed - content_type);
        }
        line = line_feed + 1;
    }
    return NULL;
}

#define HEXVAL(c) (isdigit (c) ? (c) - '0' : tolower (c) - 'a' + 10)

static size_t
hex_decode (const char *in, size_t len, char *out)
{
    register const unsigned char *inptr = (const unsigned char *)in;
    register unsigned char *outptr = (unsigned char *)out;
    const unsigned char *inend = inptr + len;

    while (inptr < inend) {
        if (*inptr == '%') {
            if (isxdigit(inptr[1]) && isxdigit(inptr[2])) {
                *outptr++ = HEXVAL(inptr[1]) * 16 + HEXVAL(inptr[2]);
                inptr += 3;
            } else
                *outptr++ = *inptr++;
        } else
            *outptr++ = *inptr++;
    }

    *outptr = '\0';

    return ((char *) outptr) - out;
}

static bool
get_rfc2231_values (const char *in, char **charset, char **language, char **value)
{
    const char *charset_begin, *charset_end;
    const char *language_begin, *language_end;
    const char *value_begin, *value_end;

    charset_begin = in;
    charset_end = strchr(charset_begin, '\'');

    language_begin = charset_end ? charset_end + 1 : charset_begin;
    language_end = strchr(language_begin, '\'');

    value_begin = language_end ? language_end + 1 : language_begin;
    value_end = value_begin;
    while (*value_end && (*value_end != ';' && *value_end != '\n'))
        value_end++;

    if (charset_end)
        *charset = strndup(charset_begin, charset_end - charset_begin);
    if (language_end)
        *language = strndup(language_begin, language_end - language_begin);
    if (value_end)
        *value = strndup(value_begin, value_end - value_begin);

    return true;
}

static char *
get_rfc2231_filename (const char *in)
{
    char *charset = NULL, *language = NULL, *value = NULL;
    char *filename = NULL;

    get_rfc2231_values(in, &charset, &language, &value);
    if (value) {
        filename = malloc(strlen(value) + 1);
        hex_decode(value, strlen(value), filename);
    }

    return filename;
}

static char *
get_filename (const char *value)
{
    const char *filename;
    const char *quote_end;
    char quote;

    while (*value != '\0' && isspace(*value))
        value++;
    if (*value == '\0')
        return NULL;

    if (strncasecmp("filename", value, strlen("filename")))
        return NULL;

    filename = value + strlen("filename");

    if (*filename == '*') { /* RFC 2231 */
        filename++;
        if (*filename == '=') {
            filename++;
            return get_rfc2231_filename(filename);
        } else {
        }
    } else if (*filename == '=') {
        filename++;
        if (*filename == '\'') {
            quote = '\'';
        } else if (*filename == '"') {
            quote = '"';
        } else {
            const char *end = filename;
            while (*end != '\0' && !isspace(*end))
                end++;
            return strndup(filename, end - filename);
        }
    }
    filename++;
    quote_end = strchr(filename, quote);
    if (!quote_end)
        return NULL;

    return strndup(filename, quote_end - filename);
}

#define CONTENT_DISPOSITION_STRING "Content-Disposition:"
bool
mz_utils_get_content_disposition (const char *contents,
                                  unsigned int contents_length,
                                  char **type, char **filename)
{
    const char *line_feed;
    const char *line = contents;

    *type = NULL;
    *filename = NULL;

    while ((line_feed = strchr(line, '\n'))) {
        if (!strncasecmp(CONTENT_DISPOSITION_STRING,
                         line,
                         strlen(CONTENT_DISPOSITION_STRING))) {
            const char *value;
            const char *semicolon;
            value = line + strlen(CONTENT_DISPOSITION_STRING);
            while (*value == ' ') {
                value++;
            }

            semicolon = strchr(value, ';');
            if (!semicolon) {
                *type = strndup(value, line_feed - value);
                return true;
            }

            *type = strndup(value, semicolon - value);
            semicolon++;
            while (*semicolon == ' ') {
                semicolon++;
            }

            *filename = get_filename(semicolon);

            return true;
        }
        line = line_feed + 1;
    }
    return false;
}

const char *
mz_utils_get_attachment_body_place (const char *contents, unsigned int *size)
{
    char *start, *end;
    *size = 0;

    start = strstr(contents, "\n\n");
    if (!start)
        return NULL;

    start += 2;

    end = strstr(start, "\n\n");
    if (!end)
        return NULL;

    *size = end - start;

    return start;
}

const char *
mz_utils_get_decoded_attachment_body (const char *contents, unsigned int *size)
{
    const char *body;
    unsigned int encoded_body_length = 0;
    int state = 0;
    unsigned int save = 0;

    *size = 0;

    body = mz_utils_get_attachment_body_place(contents, &encoded_body_length);
    if (!body)
        return NULL;

    *size = mz_base64_decode_step(body, encoded_body_length,
                                  (unsigned char *)body, &state, &save);
    return body;
}

bool
mz_utils_parse_body (const char *body, const char *boundary)
{
    char *boundary_line;
    const char *p;
    unsigned int boundary_line_length;
    const char *start_boundary, *end_boundary;

    if (!boundary || !body)
        return false;

    boundary_line_length = strlen(boundary) + 4;
    boundary_line = malloc(boundary_line_length);

    sprintf(boundary_line, "--%s\n", boundary);

    p = body;
    while ((p = strstr(p, boundary_line))) {
        char *type = NULL;
        char *filename = NULL;

        p += boundary_line_length;
        start_boundary = p;

        end_boundary = strstr(p, boundary_line);
        if (!end_boundary)
            break;
        if (mz_utils_get_content_disposition(start_boundary,
                                             end_boundary - start_boundary,
                                             &type,
                                             &filename) &&
            !strcasecmp(type, "attachment") && filename) {
            const char *attachment;
            unsigned int length;
            attachment = mz_utils_get_decoded_attachment_body(start_boundary, &length);

            if (attachment) {
                mz_attachment_new(filename, attachment, length);
            }
        }
        free(type);
        free(filename);
        p = end_boundary;
        if (!strncmp(p + boundary_line_length, "--", 2))
            break;
    }
    free(boundary_line);

    return true;
}

