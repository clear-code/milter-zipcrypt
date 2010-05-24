/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <string.h>
#include "mz-utils.h"

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

#define CONTENT_DISPOSITION_STRING "Content-Disposition:"
static char *
get_filename (const char *value, unsigned int length)
{
    const char *filename;
    const char *quote_end;
    char quote;

    if (strncasecmp("filename=", value, strlen("filename=")))
        return NULL;

    filename = value + strlen("filename=");

    if (*filename == '\'')
        quote = '\'';
    else if (*filename == '"')
        quote = '"';
    else
        return strndup(filename, length - strlen("filename="));

    filename++;
    quote_end = strchr(filename, quote);
    if (!quote_end)
        return NULL;

    return strndup(filename, quote_end - filename);
}

bool
mz_utils_get_content_disposition (const char *contents, char **type, char **filename)
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

            *filename = get_filename(semicolon, line_feed - semicolon);

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