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
            char *content_type;
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
            char *content_type;
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

