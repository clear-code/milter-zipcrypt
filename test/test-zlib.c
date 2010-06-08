/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>
#include <zlib.h>
#include <time.h>

#include "mz-test-utils.h"

void test_init (void);
void test_compress (void);

typedef struct _TestZipHeader
{
    unsigned char signature[4];
    unsigned char need_version[2];
    unsigned char flags[2];
    unsigned char compression_method[2];
    unsigned char last_modified_time[2];
    unsigned char last_modified_date[2];
    unsigned char crc[4];
    unsigned char compressed_size[4];
    unsigned char uncompressed_size[4];
    unsigned char filename_length[2];

    /* unsigned char extra_field_length[2]; */

} TestZipHeader;

static z_stream zlib_stream;
static TestZipHeader *header;

void
setup (void)
{
    header = NULL;
    cut_set_fixture_data_dir("fixtures", NULL);
    memset(&zlib_stream, 0, sizeof(zlib_stream));
}

void
teardown (void)
{
    deflateEnd(&zlib_stream);
    if (header)
        free(header);
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
                       MAX_WBITS, /* windowBits ? */
                       9, /* Use maximum memory for optimal speed.*/
                       Z_DEFAULT_STRATEGY);
    assert_z_ok(ret);
}

static TestZipHeader *
create_zip_header (const char *path)
{
    const char *raw_data;
    unsigned int raw_data_length;
    time_t last_modification;
    struct tm tm;
    unsigned short msdos_time, msdos_date;
    unsigned short filename_length;
    uLong crc;
    TestZipHeader *header;

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

    header->compression_method[0] = (char)Z_DEFLATED & 0xff;
    header->compression_method[1] = (char)(Z_DEFLATED >> 8) & 0xff;

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

    header->compressed_size[0] = raw_data_length & 0xff;
    header->compressed_size[1] = (raw_data_length >> 8) & 0xff;
    header->compressed_size[2] = (raw_data_length >> 16) & 0xff;
    header->compressed_size[3] = (raw_data_length >> 24) & 0xff;

    header->uncompressed_size[0] = raw_data_length & 0xff;
    header->uncompressed_size[1] = (raw_data_length >> 8) & 0xff;
    header->uncompressed_size[2] = (raw_data_length >> 16) & 0xff;
    header->uncompressed_size[3] = (raw_data_length >> 24) & 0xff;

    filename_length = strlen(path);
    header->filename_length[0] = filename_length & 0xff;
    header->filename_length[1] = (filename_length >> 8) & 0xff;

    return header;
}

void
test_compress (void)
{
#define BUFFER_SIZE 4096
    char compressed_data[BUFFER_SIZE];
    const char *raw_data;
    const char *expected_compressed_data;
    unsigned int raw_data_length;
    unsigned int expected_compressed_data_length;
    int ret;

    test_init();

    raw_data = mz_test_utils_load_data("body", &raw_data_length);
    cut_assert_not_null(raw_data);

    zlib_stream.next_in = (Bytef*)raw_data;
    zlib_stream.avail_in = raw_data_length;

    zlib_stream.next_out = (Bytef*)compressed_data;
    zlib_stream.avail_out = BUFFER_SIZE;

    expected_compressed_data = mz_test_utils_load_data("body.zip", &expected_compressed_data_length);
    cut_assert_not_null(expected_compressed_data);

    header = create_zip_header("body");
    cut_assert_equal_memory(expected_compressed_data, sizeof(TestZipHeader),
                            header, sizeof(*header));

    expected_compressed_data += sizeof(*header);

    while ((ret = deflate(&zlib_stream, Z_FINISH)) == Z_OK) {
        unsigned int written_bytes;

        written_bytes = BUFFER_SIZE - zlib_stream.avail_out;
        cut_assert_equal_memory(expected_compressed_data, expected_compressed_data_length,
                                compressed_data, written_bytes);

        expected_compressed_data += written_bytes;
        zlib_stream.next_out = (Bytef*)compressed_data;
        zlib_stream.avail_out = BUFFER_SIZE;
    }
}

