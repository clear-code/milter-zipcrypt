/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <string.h>
#include <stdlib.h>
#include "mz-attachment.h"

MzAttachment *
mz_attachment_new (const char *charset,
                   const char *filename,
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

    attachment->filename = attachment_filename;
    attachment->data = data;
    attachment->data_length = data_length;

    return attachment;

fail:
    free(filename_charset);
    free(attachment_filename);
    free(attachment);

    return NULL;
}

void
mz_attachment_free (MzAttachment *attachment)
{
    if (!attachment)
        return;

    free(attachment->charset);
    free(attachment->filename);
    free(attachment);
}
