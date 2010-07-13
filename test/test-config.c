/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>

#include "mz-test-utils.h"
#include "mz-config.h"

void test_load (void);

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
    config = mz_config_load(mz_test_utils_build_fixture_data_path("config"));
}

