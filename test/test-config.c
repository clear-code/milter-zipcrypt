/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>

#include "mz-test-utils.h"
#include "mz-config.h"

void test_load (void);
void test_get_string (void);
void test_set_string (void);

static MzConfig *config;

void
setup (void)
{
    config = NULL;
}

void
teardown (void)
{
    mz_config_free(config);
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

