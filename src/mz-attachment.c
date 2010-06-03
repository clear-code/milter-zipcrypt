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
    if (filename_charset)
        free(filename_charset);
    if (attachment_filename)
        free(attachment_filename);
    free(attachment);

    return NULL;
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

static MzAttachments *
mz_attachments_last (MzAttachments *attachments)
{
    if (attachments) {
        while (attachments->next)
            attachments = attachments->next;
    }

    return attachments;
}

MzAttachments *
mz_attachments_append (MzAttachments *attachments, MzAttachment  *attachment)
{
    MzAttachments *new;
    MzAttachments *last;

    new = malloc(sizeof(*new));
    if (!new)
        return NULL;

    new->attachment = attachment;
    new->next = NULL;

    if (attachments) {
        last = mz_attachments_last(attachments);
        last->next = new;
        return attachments;
    } else {
        return new;
    }
}

void
mz_attachments_free (MzAttachments *attachments)
{
    while (attachments) {
        MzAttachments *next;
        next = attachments->next;
        mz_attachment_free(attachments->attachment);
        free(attachments);
        attachments = next;
    }
}
