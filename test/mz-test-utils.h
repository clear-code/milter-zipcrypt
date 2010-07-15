/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#ifndef __MZ_TEST_UTILS_H__
#define __MZ_TEST_UTILS_H__

#include <time.h>

const char  *mz_test_utils_load_data              (const char     *path,
                                                   unsigned int   *size);
const char  *mz_test_utils_build_fixture_data_path(const char     *path, ...);
time_t       mz_test_utils_get_last_modified_time (const char     *path);
unsigned int mz_test_utils_get_file_attributes    (const char     *path);
const char  *mz_test_utils_create_temporary_config_file
                                                  (const char *contents);

#endif /* __MZ_TEST_UTILS_H__ */
