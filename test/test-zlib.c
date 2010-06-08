/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>
#include <zlib.h>
#include <time.h>

#include "mz-test-utils.h"
#include "mz-zip.h"

void test_init (void);
void test_compress (void);

static z_stream zlib_stream;
static MzZipHeader *actual_header;

void
setup (void)
{
    actual_header = NULL;
    cut_set_fixture_data_dir("fixtures", NULL);
    memset(&zlib_stream, 0, sizeof(zlib_stream));
}

void
teardown (void)
{
    deflateEnd(&zlib_stream);
    if (actual_header)
        free(actual_header);
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
                       -14, /* zip on Linux seems to use -14. */
                       9, /* Use maximum memory for optimal speed.*/
                       Z_DEFAULT_STRATEGY);
    assert_z_ok(ret);
}

#define GET_16BIT_VALUE(x) (((x[0]) & 0xff) | (((x[1]) << 8)))
static void
assert_equal_zip_header (MzZipHeader *expected, MzZipHeader *actual)
{
    unsigned short actual_filename_length;
    unsigned short expected_filename_length;

#define CHECK_PARAM(name)                                           \
    cut_assert_equal_memory(expected->name, sizeof(expected->name), \
                            actual->name, sizeof(actual->name));

    CHECK_PARAM(signature);
    CHECK_PARAM(need_version);
    CHECK_PARAM(flags);
    CHECK_PARAM(compression_method);
    CHECK_PARAM(last_modified_time);
    CHECK_PARAM(last_modified_date);
    CHECK_PARAM(crc);
    CHECK_PARAM(compressed_size);
    CHECK_PARAM(uncompressed_size);
    CHECK_PARAM(filename_length);
    CHECK_PARAM(extra_field_length);

    expected_filename_length = GET_16BIT_VALUE(expected->filename_length);
    actual_filename_length = GET_16BIT_VALUE(actual->filename_length);
}

static MzZipHeader *
create_zip_header (const char *path, unsigned int compressed_size)
{
    const char *raw_data;
    unsigned int raw_data_length;
    unsigned short extra_field_length;
    time_t last_modification;
    struct tm tm;
    unsigned short msdos_time, msdos_date;
    unsigned short filename_length;
    uLong crc;
    MzZipHeader *header;

    header = malloc(sizeof(*header));

    raw_data = mz_test_utils_load_data(path, &raw_data_length);

    header->signature[0] = 0x50;
    header->signature[1] = 0x4b;
    header->signature[2] = 0x03;
    header->signature[3] = 0x04;

    header->need_version[0] = 0x14;
    header->need_version[1] = 0x00;

    header->flags[0] = 0x04; /* Fast compression mode */
    header->flags[1] = 0x00;

    header->compression_method[0] = Z_DEFLATED & 0xff;
    header->compression_method[1] = (Z_DEFLATED >> 8) & 0xff;

    last_modification = mz_test_utils_get_last_modified_time(path);
    localtime_r(&last_modification, &tm);

    msdos_time = (tm.tm_hour << 11) | (tm.tm_min << 5) | ((tm.tm_sec + 1) >> 1);
    msdos_date = ((tm.tm_year - 80) << 9) | ((tm.tm_mon  + 1) << 5) | tm.tm_mday;

    header->last_modified_time[0] = msdos_time & 0xff;
    header->last_modified_time[1] = (msdos_time >> 8) & 0xff;

    header->last_modified_date[0] = msdos_date & 0xff;
    header->last_modified_date[1] = (msdos_date >> 8) & 0xff;

    crc = crc32(0, NULL, 0);
    crc = crc32(crc, (unsigned char*)raw_data, raw_data_length);
    header->crc[0] = crc & 0xff;
    header->crc[1] = (crc >> 8) & 0xff;
    header->crc[2] = (crc >> 16) & 0xff;
    header->crc[3] = (crc >> 24) & 0xff;

    header->compressed_size[0] = compressed_size & 0xff;
    header->compressed_size[1] = (compressed_size >> 8) & 0xff;
    header->compressed_size[2] = (compressed_size >> 16) & 0xff;
    header->compressed_size[3] = (compressed_size >> 24) & 0xff;

    header->uncompressed_size[0] = raw_data_length & 0xff;
    header->uncompressed_size[1] = (raw_data_length >> 8) & 0xff;
    header->uncompressed_size[2] = (raw_data_length >> 16) & 0xff;
    header->uncompressed_size[3] = (raw_data_length >> 24) & 0xff;

    filename_length = strlen(path);
    header->filename_length[0] = filename_length & 0xff;
    header->filename_length[1] = (filename_length >> 8) & 0xff;

    extra_field_length = 28;
    header->extra_field_length[0] = extra_field_length & 0xff;
    header->extra_field_length[1] = (extra_field_length >> 8) & 0xff;

    return header;
}

void
test_compress (void)
{
#define BUFFER_SIZE 4096
    char compressed_data[8192]; /* enough space to store the data */
    const char *raw_data;
    const char *expected_compressed_data;
    unsigned int raw_data_length;
    unsigned int expected_compressed_data_length;
    unsigned int compressed_data_length = 0;
    int ret = Z_OK;
    MzZipHeader expected_header;

    test_init();

    raw_data = mz_test_utils_load_data("body", &raw_data_length);
    cut_assert_not_null(raw_data);

    zlib_stream.next_in = (Bytef*)raw_data;
    zlib_stream.avail_in = raw_data_length;

    zlib_stream.next_out = (Bytef*)compressed_data;
    zlib_stream.avail_out = BUFFER_SIZE;

    while (ret  == Z_OK || ret == Z_STREAM_END) {
        unsigned int written_bytes;

        ret = deflate(&zlib_stream, Z_FINISH);

        written_bytes = BUFFER_SIZE - zlib_stream.avail_out;
        compressed_data_length += written_bytes;
        zlib_stream.next_out = (Bytef*)compressed_data + compressed_data_length;
        zlib_stream.avail_out = BUFFER_SIZE;
        if (ret == Z_STREAM_END)
            break;
    }

    expected_compressed_data = mz_test_utils_load_data("body.zip", &expected_compressed_data_length);
    cut_assert_not_null(expected_compressed_data);

    actual_header = create_zip_header("body", compressed_data_length);
    memcpy(&expected_header, expected_compressed_data, sizeof(expected_header));
    expected_compressed_data += sizeof(expected_header);

    assert_equal_zip_header(&expected_header, actual_header);

    cut_assert_equal_memory("body", strlen("body"),
                            expected_compressed_data, GET_16BIT_VALUE(expected_header.filename_length));
    expected_compressed_data += GET_16BIT_VALUE(expected_header.filename_length);
    expected_compressed_data += GET_16BIT_VALUE(expected_header.extra_field_length);

    cut_assert_equal_memory(expected_compressed_data, expected_compressed_data_length,
                            compressed_data, compressed_data_length);
}

