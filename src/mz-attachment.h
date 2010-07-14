/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_ATTACHMENT_H__
#define __MZ_ATTACHMENT_H__

#include <stdbool.h>

typedef struct _MzAttachment MzAttachment;

struct _MzAttachment
{
    char *charset;
    char *filename;
    const char *boundary_start_position;
    const char *data;
    unsigned int data_length;
    time_t last_modified_time; /* Use only for test */
    unsigned int file_attributes; /* Use only for test */
};

typedef struct _MzAttachments MzAttachments;

MzAttachment *mz_attachment_new  (const char *charset,
                                  const char *filename,
                                  const char *boundary_start_position,
                                  const char *data,
                                  unsigned int data_length);
void          mz_attachment_free (MzAttachment *attachment);

char         *mz_attachment_get_filename_without_extension
                                 (MzAttachment *attachment);

#endif /* __MZ_ATTACHMENT_H__ */

