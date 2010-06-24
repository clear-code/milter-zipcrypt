/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <iconv.h>
#include "mz-utils.h"
#include "mz-attachment.h"
#include "mz-base64.h"

#define CRLF "\r\n"
static size_t CRLF_LENGTH = sizeof(CRLF) - 1;

#define CONTENT_TYPE_STRING "Content-type:"

char *
mz_utils_get_content_type (const char *contents)
{
    const char *line_feed;
    const char *line = contents;

    while ((line_feed = strstr(line, CRLF))) {
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
        line = line_feed + CRLF_LENGTH;
    }
    return NULL;
}

#define CONTENT_TRANSFER_ENCODING_STRING "Content-Transfer-Encoding:"
char *
mz_utils_get_content_transfer_encoding (const char *contents)
{
    const char *line_feed;
    const char *line = contents;

    while ((line_feed = strstr(line, CRLF))) {
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
        line = line_feed + CRLF_LENGTH;
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

static size_t
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
    while (*value_end && (*value_end != ';' && *value_end != '\r'))
        value_end++;

    if (charset_end && *charset == NULL)
        *charset = strndup(charset_begin, charset_end - charset_begin);
    if (language_end)
        *language = strndup(language_begin, language_end - language_begin);
    if (value_end)
        *value = strndup(value_begin, value_end - value_begin);

    return (value_end - in);
}

static size_t
get_rfc2231_filename (const char *in, char **charset, char **filename)
{
    char *language = NULL, *value = NULL;
    size_t processed_length;

    processed_length = get_rfc2231_values(in, charset, &language, &value);
    if (value) {
        *filename = malloc(strlen(value) + 1);
        hex_decode(value, strlen(value), *filename);
    }

    return processed_length;
}

static size_t
get_filename (const char *in, char **charset, char **filename)
{
    const char *p;
    const char *quote_end;
    char quote;

    while (*in != '\0' && isspace(*in))
        in++;
    if (*in == '\0')
        return 0;

    if (strncasecmp("filename", in, strlen("filename")))
        return 0;

    p = in + strlen("filename");

    if (*p == '*') { /* RFC 2231 */
        p++;
        if (*p == '=') {
            p++;
            return get_rfc2231_filename(p, charset, filename);
        } else if (*p >= '0' && *p <= '9'){
            size_t processed_length;
            char *rest_filename = NULL;
            p++;
            if (*p == '*')
                p++;
            if (*p != '=')
                return 0;
            p++; /* = */
            processed_length = get_rfc2231_filename(p, charset, filename);
            p += processed_length + 1;
            get_filename(p, charset, &rest_filename);
            if (rest_filename) {
                char *new_filename = malloc(strlen(*filename) + strlen(rest_filename));
                sprintf(new_filename, "%s%s", *filename, rest_filename);
                free(*filename);
                free(rest_filename);
                *filename = new_filename;
                return strlen(new_filename);
            }
            return *filename ? strlen(*filename) : 0;
        } else {
            return 0;
        }
    } else if (*p == '=') {
        p++;
        if (*p == '\'') {
            quote = '\'';
        } else if (*p == '"') {
            quote = '"';
        } else {
            const char *end = p;
            while (*end != '\0' && !isspace(*end))
                end++;
            *filename = strndup(p, end - p);
            return (end - p);
        }
    } else {
        return 0;
    }
    p++;
    quote_end = strchr(p, quote);
    if (!quote_end)
        return 0;

    *filename = strndup(p, quote_end - p);
    return (quote_end - p);
}

#define CONTENT_DISPOSITION_STRING "Content-Disposition:"
bool
mz_utils_get_content_disposition (const char *contents,
                                  unsigned int contents_length,
                                  char **type,
                                  char **charset,
                                  char **filename)
{
    const char *line_feed;
    const char *line = contents;

    *type = NULL;
    *filename = NULL;

    while ((line_feed = strstr(line, CRLF))) {
        if (line_feed - contents > contents_length)
            return false;

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

            get_filename(semicolon, charset, filename);

            return true;
        }
        line = line_feed + CRLF_LENGTH;
    }
    return false;
}

const char *
mz_utils_get_attachment_body_place (const char *contents, unsigned int *size)
{
    char *start, *end;
    *size = 0;

    start = strstr(contents, CRLF CRLF);
    if (!start)
        return NULL;

    start += CRLF_LENGTH * 2;

    end = strstr(start, CRLF CRLF);
    if (!end)
        return NULL;

    *size = end - start;

    return start;
}

const char *
mz_utils_get_decoded_attachment_body (const char *contents, unsigned int *size)
{
    const char *body;
    char *encoding;
    unsigned int encoded_body_length = 0;
    int state = 0;
    unsigned int save = 0;

    *size = 0;

    body = mz_utils_get_attachment_body_place(contents, &encoded_body_length);
    if (!body)
        return NULL;

    encoding = mz_utils_get_content_transfer_encoding(contents);
    if (!strncasecmp(encoding, "base64", 6)) {
        *size = mz_base64_decode_step(body, encoded_body_length,
                                      (unsigned char *)body, &state, &save);
    } else {
        *size = encoded_body_length;
    }

    return body;
}

MzList *
mz_utils_extract_attachments (const char *body, const char *boundary)
{
    MzList *attachments = NULL;
    char *boundary_line;
    const char *p;
    unsigned int boundary_line_length;
    const char *start_boundary, *end_boundary;

    if (!boundary || !body)
        return NULL;

    boundary_line_length = strlen(boundary) + 2;
    boundary_line = malloc(boundary_line_length + 1);

    sprintf(boundary_line, "--%s", boundary);

    p = body;
    while ((p = strstr(p, boundary_line))) {
        char *type = NULL;
        char *charset = NULL;
        char *filename = NULL;

        p += boundary_line_length;
        p += CRLF_LENGTH;
        start_boundary = p;

        end_boundary = strstr(p, boundary_line);
        if (!end_boundary)
            break;
        if (mz_utils_get_content_disposition(start_boundary,
                                             end_boundary - start_boundary,
                                             &type,
                                             &charset,
                                             &filename) &&
            !strcasecmp(type, "attachment") && filename) {
            const char *attachment;
            unsigned int length;
            attachment = mz_utils_get_decoded_attachment_body(start_boundary, &length);

            if (attachment) {
                attachments = mz_list_append(attachments,
                                             mz_attachment_new(charset,
                                                               filename,
                                                               start_boundary,
                                                               attachment,
                                                               length));
            }
        }
        free(type);
        free(filename);
        p = end_boundary;
        if (!strncmp(p + boundary_line_length, "--", 2))
            break;
    }
    free(boundary_line);

    return attachments;
}

