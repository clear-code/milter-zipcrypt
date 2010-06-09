/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>
#include <zlib.h>

#include "mz-test-utils.h"
#include "mz-zip.h"

void test_compress_in_memory (void);

static MzZipHeader *actual_header;
static MzZipCentralDirectoryRecord *actual_directory_record;
static MzZipEndOfCentralDirectoryRecord *actual_end_of_directory_record;

void
setup (void)
{
    actual_header = NULL;
    actual_directory_record = NULL;
    actual_end_of_directory_record = NULL;
    cut_set_fixture_data_dir("fixtures", NULL);
}

void
teardown (void)
{
    if (actual_header)
        free(actual_header);
    if (actual_directory_record)
        free(actual_directory_record);
    if (actual_end_of_directory_record)
        free(actual_end_of_directory_record);
}

#define GET_16BIT_VALUE(x) (((x[0]) & 0xff) | (((x[1]) << 8)))
#define GET_32BIT_VALUE(x) (((x[0]) & 0xff) | (((x[1]) << 8)) | (((x[2] << 16)) | (((x[3]) << 24))))
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

static void
assert_equal_zip_central_directory_record (MzZipCentralDirectoryRecord *expected, MzZipCentralDirectoryRecord *actual)
{
#define CHECK_PARAM(name)                                           \
    cut_assert_equal_memory(expected->name, sizeof(expected->name), \
                            actual->name, sizeof(actual->name));

    CHECK_PARAM(signature);
    CHECK_PARAM(made_version);
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
    CHECK_PARAM(file_comment_length);
    CHECK_PARAM(start_disk_num);
    CHECK_PARAM(internal_file_attributes);
    CHECK_PARAM(external_file_attributes);
    CHECK_PARAM(header_offset);
}

static void
assert_equal_zip_end_of_central_directory_record (MzZipEndOfCentralDirectoryRecord *expected,
                                                  MzZipEndOfCentralDirectoryRecord *actual)
{
#define CHECK_PARAM(name)                                           \
    cut_assert_equal_memory(expected->name, sizeof(expected->name), \
                            actual->name, sizeof(actual->name));

    CHECK_PARAM(signature);
    CHECK_PARAM(num_disk);
    CHECK_PARAM(start_disk_num);
    CHECK_PARAM(total_disk_num);
    CHECK_PARAM(total_entry_num);
    CHECK_PARAM(entry_size);
    CHECK_PARAM(offset);
    CHECK_PARAM(comment_length);
}

void
test_compress_in_memory (void)
{
    char compressed_data[8192]; /* enough space to store the data */
    const char *raw_data;
    const char *expected_compressed_data;
    unsigned int raw_data_length;
    unsigned int expected_compressed_data_length;
    unsigned int compressed_data_length;
    int guessed_data_type;
    time_t last_modified_time;
    MzZipHeader expected_header;
    MzZipCentralDirectoryRecord expected_directory_record;
    MzZipEndOfCentralDirectoryRecord expected_end_of_directory_record;

    raw_data = mz_test_utils_load_data("body", &raw_data_length);
    cut_assert_not_null(raw_data);

    compressed_data_length = mz_zip_compress_in_memory(raw_data,
                                                       raw_data_length,
                                                       (char**)&compressed_data,
                                                       &guessed_data_type);
    cut_assert_not_equal_int(0, compressed_data_length);

    expected_compressed_data = mz_test_utils_load_data("body.zip", &expected_compressed_data_length);
    cut_assert_not_null(expected_compressed_data);

    last_modified_time = mz_test_utils_get_last_modified_time("body");
    actual_header = mz_zip_create_header("body",
                                         raw_data,
                                         raw_data_length,
                                         last_modified_time,
                                         compressed_data_length);
    memcpy(&expected_header, expected_compressed_data, sizeof(expected_header));
    expected_compressed_data += sizeof(expected_header);

    assert_equal_zip_header(&expected_header, actual_header);

    cut_assert_equal_memory("body", strlen("body"),
                            expected_compressed_data, GET_16BIT_VALUE(expected_header.filename_length));
    expected_compressed_data += GET_16BIT_VALUE(expected_header.filename_length);
    expected_compressed_data += GET_16BIT_VALUE(expected_header.extra_field_length);

    cut_assert_equal_memory(expected_compressed_data, GET_32BIT_VALUE(expected_header.compressed_size),
                            compressed_data, compressed_data_length);
    expected_compressed_data += GET_32BIT_VALUE(expected_header.compressed_size);

    actual_directory_record = mz_zip_create_central_directory_record("body",
                                                                     actual_header,
                                                                     mz_test_utils_get_file_attributes("body"),
                                                                     guessed_data_type);
    memcpy(&expected_directory_record, expected_compressed_data, sizeof(expected_directory_record));
    expected_compressed_data += sizeof(expected_directory_record);

    assert_equal_zip_central_directory_record(&expected_directory_record, actual_directory_record);

    cut_assert_equal_memory("body", strlen("body"),
                            expected_compressed_data, GET_16BIT_VALUE(expected_directory_record.filename_length));

    expected_compressed_data += GET_16BIT_VALUE(expected_directory_record.filename_length);
    expected_compressed_data += GET_16BIT_VALUE(expected_directory_record.extra_field_length);

    actual_end_of_directory_record = mz_zip_create_end_of_central_directory_record(actual_directory_record);
    memcpy(&expected_end_of_directory_record,
           expected_compressed_data,
           sizeof(expected_end_of_directory_record));
    assert_equal_zip_end_of_central_directory_record(&expected_end_of_directory_record,
                                                     actual_end_of_directory_record);
}

