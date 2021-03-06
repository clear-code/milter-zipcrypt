/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <string.h>
#include <stdlib.h>
#include "mz-attachment.h"
#include "mz-utils.h"

MzAttachment *
mz_attachment_new (const char *charset,
                   const char *filename,
                   const char *boundary_start_position,
                   const char *data,
                   unsigned int data_length)
{
    MzAttachment *attachment = NULL;
    char *attachment_filename = NULL;
    char *filename_charset = NULL;

    attachment = malloc(sizeof(*attachment));
    if (!attachment)
        return NULL;

    if (charset) {
        filename_charset = strdup(charset);
        if (!filename_charset)
            goto fail;
    }

    attachment_filename = strdup(filename);
    if (!attachment_filename)
        goto fail;

    attachment->charset = filename_charset;
    attachment->filename = attachment_filename;
    attachment->boundary_start_position = boundary_start_position;
    attachment->data = data;
    attachment->data_length = data_length;
    attachment->last_modified_time = 0;
    attachment->file_attributes = 0;

    return attachment;

fail:
    if (filename_charset)
        free(filename_charset);
    if (attachment_filename)
        free(attachment_filename);
    free(attachment);

    return NULL;
}

char *
mz_attachment_get_filename_without_extension (MzAttachment *attachment)
{
    if (!attachment)
        return NULL;

    return mz_utils_get_filename_without_extension(attachment->filename);
}

void
mz_attachment_free (MzAttachment *attachment)
{
    if (!attachment)
        return;

    if (attachment->charset)
        free(attachment->charset);
    if (attachment->filename)
        free(attachment->filename);
    free(attachment);
}

