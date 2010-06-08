/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#ifndef __MZ_TEST_UTILS_H__
#define __MZ_TEST_UTILS_H__

#include <stdbool.h>

const char  *mz_test_utils_load_data              (const char     *path,
                                                   unsigned int   *size);
bool         mz_test_utils_get_last_modified_time (const char     *path,
                                                   unsigned short *msdos_time,
                                                   unsigned short *msdos_date);
unsigned int mz_test_utils_get_file_attributes    (const char     *path);

#endif /* __MZ_TEST_UTILS_H__ */
