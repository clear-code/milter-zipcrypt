/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_ATTACHMENT_H__
#define __MZ_ATTACHMENT_H__

#include <stdbool.h>

typedef struct _MzAttachment MzAttachment;

struct _MzAttachment
{
    char *filename;
    const char *data;
    unsigned int data_length;
};

MzAttachment *mz_attachment_new (const char *filename,
                                 const char *data,
                                 unsigned int data_length);

#endif /* __MZ_ATTACHMENT_H__ */

