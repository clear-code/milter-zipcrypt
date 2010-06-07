/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>

#include <zlib.h>

void test_init (void);

static z_stream zlib_stream;

void
setup (void)
{
    memset(&zlib_stream, 0, sizeof(zlib_stream));
}

void
teardown (void)
{
    deflateEnd(&zlib_stream);
}

static void
assert_z_ok (int zlib_code)
{
    cut_assert_equal_int(Z_OK, zlib_code);
}

void
test_init (void)
{
    int compression_level = 1;
    int ret;

    ret = deflateInit2(&zlib_stream,
                       compression_level,
                       Z_DEFLATED,
                       -8, /* windowBits ? */
                       9, /* Use maximum memory for optimal speed.*/
                       Z_DEFAULT_STRATEGY);
    assert_z_ok(ret);
}

