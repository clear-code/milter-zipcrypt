/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <string.h>
#include <stdlib.h>
#include "mz-attachment.h"

MzAttachment *
mz_attachment_new (const char *filename,
                   const char *data,
                   unsigned int data_length)
{
    MzAttachment *attachment;
    char *attachment_filename;

    attachment = malloc(sizeof(*attachment));
    if (!attachment)
        return NULL;

    attachment_filename = strdup(filename);
    if (!attachment_filename) {
        free(attachment);
        return NULL;
    }

    attachment->filename = attachment_filename;
    attachment->data = data;
    attachment->data_length = data_length;

    return attachment;
}

void
mz_attachment_free (MzAttachment *attachment)
{
    if (!attachment)
        return;

    free(attachment->filename);
    free(attachment);
}
