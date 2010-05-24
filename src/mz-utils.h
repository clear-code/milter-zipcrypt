/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_UTILS_H__
#define __MZ_UTILS_H__

#include <stdbool.h>

char *mz_utils_get_content_type (const char *contents);
char *mz_utils_get_content_transfer_encoding (const char *contents);
bool  mz_utils_get_content_disposition (const char *contents, char **type, char **filename);

#endif /* __MZ_UTILS_H__ */

