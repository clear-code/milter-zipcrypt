/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>
#include <string.h>
#include <stdio.h>
#include "mz-utils.h"

void test_get_content_type (void);
void test_get_content_transfer_encoding (void);
void test_get_content_disposition (void);

void
setup (void)
{
    cut_set_fixture_data_dir("fixtures", NULL);
}

void
teardown (void)
{
}

void
test_get_content_type (void)
{
    char *content_type;
    const char *content;

    content = cut_get_fixture_data_string("attachment", NULL);
    cut_assert_not_null(content);

    content_type = mz_utils_get_content_type(content);
    cut_take_string(content_type);
    cut_assert_equal_string("image/png", content_type);
}

void
test_get_content_transfer_encoding (void)
{
    char *content_transfer_encoding;
    const char *content;

    content = cut_get_fixture_data_string("attachment", NULL);
    cut_assert_not_null(content);

    content_transfer_encoding = mz_utils_get_content_transfer_encoding(content);
    cut_take_string(content_transfer_encoding);
    cut_assert_equal_string("base64", content_transfer_encoding);
}

void
test_get_content_disposition (void)
{
    char *type = NULL, *filename = NULL;
    const char *content;

    cut_take_string(type);
    cut_take_string(filename);

    content = cut_get_fixture_data_string("attachment", NULL);
    cut_assert_not_null(content);

    cut_assert_true(mz_utils_get_content_disposition(content, &type, &filename));
    cut_assert_equal_string("attachment", type);
    cut_assert_equal_string("t.png", filename);
}

