/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>

#include "mz-test-utils.h"
#include "mz-config.h"

void test_load (void);
void test_reload (void);
void test_get_string (void);
void test_set_string (void);

static MzConfig *config;
static const char *config_path;

void
setup (void)
{
    config = NULL;
    config_path = NULL;
}

void
teardown (void)
{
    mz_config_free(config);

    cut_remove_path(config_path, NULL);
}

void
test_load (void)
{
    config = mz_config_load(mz_test_utils_build_fixture_data_path("config", "normal.conf", NULL));
    cut_assert_not_null(config);
}

void
test_get_string (void)
{
    cut_trace(test_load());

    cut_assert_equal_string("/XXX/SENDMAIL",
                            mz_config_get_string(config, "sendmail_path"));
}

void
test_set_string (void)
{
    cut_trace(test_load());

    cut_assert_null(mz_config_get_string(config, "new_value"));
    mz_config_set_string(config, "new_value", "12345678X");
    cut_assert_equal_string("12345678X",
                            mz_config_get_string(config, "new_value"));
}

void
test_reload (void)
{
    config_path = mz_test_utils_create_temporary_config_file("sendmail_path = 12345\n");

    config = mz_config_load(config_path);
    cut_assert_not_null(config);

    cut_assert_equal_string("12345", mz_config_get_string(config, "sendmail_path"));

    config_path = mz_test_utils_create_temporary_config_file("sendmail_path = 54321\n");

    cut_assert_true(mz_config_reload(config));
    cut_assert_equal_string("54321", mz_config_get_string(config, "sendmail_path"));
}

