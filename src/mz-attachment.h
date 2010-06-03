/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_ATTACHMENT_H__
#define __MZ_ATTACHMENT_H__

#include <stdbool.h>

typedef struct _MzAttachment MzAttachment;

struct _MzAttachment
{
    char *charset;
    char *filename;
    const char *data;
    unsigned int data_length;
};

typedef struct _MzAttachments MzAttachments;

struct _MzAttachments
{
    MzAttachment *attachment;
    MzAttachments *next;
};

MzAttachment *mz_attachment_new  (const char *charset,
                                  const char *filename,
                                  const char *data,
                                  unsigned int data_length);
void          mz_attachment_free (MzAttachment *attachment);

#define mz_attachments_next(attachments) ((attachments) ? (((MzAttachments*)(attachments))->next) : NULL)

MzAttachments *mz_attachments_append (MzAttachments *attachments,
                                      MzAttachment  *attachment);
void           mz_attachments_free   (MzAttachments *attachments);

#endif /* __MZ_ATTACHMENT_H__ */

