/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>
#include "mz-filter.h"

void test_header (void);

void
setup (void)
{
}

void
teardown (void)
{
}

void
test_header (void)
{
    sfsistat s;

    s = mz_header(NULL, "Content-type", "multipart/mixed;  boundary=\"--Next_Part(Mon_Apr_12_13_31_44_2010_881)--\"");
    cut_assert_equal_int(1, s);
}

