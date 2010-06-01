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

MzAttachment *mz_attachment_new  (const char *charset,
                                  const char *filename,
                                  const char *data,
                                  unsigned int data_length);
void          mz_attachment_free (MzAttachment *attachment);

#endif /* __MZ_ATTACHMENT_H__ */

