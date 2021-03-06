/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_UTILS_H__
#define __MZ_UTILS_H__

#include <stdbool.h>
#include "mz-attachment.h"
#include "mz-list.h"

char *mz_utils_get_content_type (const char *contents);
char *mz_utils_get_content_transfer_encoding (const char *contents);
bool  mz_utils_get_content_disposition (const char *contents,
                                        unsigned int contents_length,
                                        char **type,
                                        char **charset,
                                        char **filename);

const char *mz_utils_get_attachment_body_place   (const char *contents,
                                                  const char *boundary,
                                                  unsigned int *size);
const char *mz_utils_get_decoded_attachment_body (const char *contents,
                                                  const char *boundary,
                                                  unsigned int *size);
MzList     *mz_utils_extract_attachments         (const char *body,
                                                  const char *boundary);
char       *mz_utils_get_filename_without_extension
                                                 (const char *filename);
#endif /* __MZ_UTILS_H__ */

