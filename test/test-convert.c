/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>

#include "mz-convert.h"

void test_convert (void);

static char *converted;

void
setup (void)
{
    converted = NULL;
}

void
teardown (void)
{
    if (converted)
        free(converted);
}

void
test_convert (void)
{
    const char *filename =
        "\x1B\x24\x42\x3F\x37\x24\x37\x24\x24\x25\x46\x25\x2D\x25" /* 新しいテキスト　ドキュメント.txt */
        "\x39\x25\x48\x1B\x28\x42\x20\x1B\x24\x42\x25\x49\x25\x2D"
        "\x25\x65\x25\x61\x25\x73\x25\x48\x1B\x28\x42\x2E\x74\x78\x74";
    size_t bytes_read, bytes_written;

    converted = mz_convert(filename, strlen(filename),
                           "UTF-8", "ISO-2022-JP",
                           &bytes_read, &bytes_written);
    cut_assert_equal_string("新しいテキスト ドキュメント.txt", converted);
}

