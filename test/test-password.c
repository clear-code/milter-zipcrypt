/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>

#include "mz-password.h"

void test_create (void);

static char *password;

void
setup (void)
{
    password = NULL;
}

void
teardown (void)
{
    if (password)
        free(password);
}

void
test_create (void)
{
    password = mz_password_create();
    cut_assert_not_null(password);
}

