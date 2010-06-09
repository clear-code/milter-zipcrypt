/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>
#include <zlib.h>

#include "mz-test-utils.h"
#include "mz-zip.h"

void test_compress (void);

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

static MzZipCentralDirectoryRecord *
create_zip_central_directory_record (const char *path, MzZipHeader *header, int data_type)
{
    MzZipCentralDirectoryRecord *record;
    void *dest, *src;
    unsigned short extra_field_length;
    unsigned int file_attributes;

    record = malloc(sizeof(*record));

    record->signature[0] = 0x50;
    record->signature[1] = 0x4b;
    record->signature[2] = 0x01;
    record->signature[3] = 0x02;

    record->made_version[0] = 0x1e;
    record->made_version[1] = 0x03;

    dest = record;
    dest += 6;
    src = header;
    src += 4;
    memcpy(dest, src, sizeof(*header) - 4);

    extra_field_length = 24;
    record->extra_field_length[0] = extra_field_length & 0xff;
    record->extra_field_length[1] = (extra_field_length >> 8) & 0xff;

    record->file_comment_length[0] = 0;
    record->file_comment_length[1] = 0;

    record->start_disk_num[0] = 0;
    record->start_disk_num[1] = 0;

    record->internal_file_attributes[0] = data_type & 0xff;
    record->internal_file_attributes[1] = (data_type >> 8) & 0xff;

    file_attributes = mz_test_utils_get_file_attributes(path);

    record->external_file_attributes[0] = file_attributes & 0xff;
    record->external_file_attributes[1] = (file_attributes >> 8) & 0xff;
    record->external_file_attributes[2] = (file_attributes >> 16) & 0xff;
    record->external_file_attributes[3] = (file_attributes >> 24) & 0xff;

    record->header_offset[0] = 0;
    record->header_offset[1] = 0;
    record->header_offset[2] = 0;
    record->header_offset[3] = 0;

    return record;
}

static MzZipEndOfCentralDirectoryRecord *
create_zip_end_of_central_directory_record (MzZipCentralDirectoryRecord *central_record)
{
    MzZipEndOfCentralDirectoryRecord *record;
    unsigned short central_record_length;

    record = malloc(sizeof(*record));

    record->signature[0] = 0x50;
    record->signature[1] = 0x4b;
    record->signature[2] = 0x05;
    record->signature[3] = 0x06;

    record->num_disk[0] = 0;
    record->num_disk[1] = 0;

    record->start_disk_num[0] = 0;
    record->start_disk_num[1] = 0;

    record->total_disk_num[0] = 0x01;
    record->total_disk_num[1] = 0;

    record->total_entry_num[0] = 0x01;
    record->total_entry_num[1] = 0;

    central_record_length = sizeof(*central_record) +
                            GET_16BIT_VALUE(central_record->filename_length) +
                            GET_16BIT_VALUE(central_record->extra_field_length);

    record->entry_size[0] = central_record_length & 0xff;
    record->entry_size[1] = (central_record_length >> 8) & 0xff;

    record->offset[0] = 0;
    record->offset[1] = 0;
    record->offset[2] = 0xa0;
    record->offset[3] = 0x12;

    record->comment_length[0] = 0;
    record->comment_length[1] = 0;

    return record;
}

void
test_compress (void)
{
    char compressed_data[8192]; /* enough space to store the data */
    const char *raw_data;
    const char *expected_compressed_data;
    unsigned int raw_data_length;
    unsigned int expected_compressed_data_length;
    unsigned int compressed_data_length;
    time_t last_modified_time;
    MzZipHeader expected_header;
    MzZipCentralDirectoryRecord expected_directory_record;
    MzZipEndOfCentralDirectoryRecord expected_end_of_directory_record;

    raw_data = mz_test_utils_load_data("body", &raw_data_length);
    cut_assert_not_null(raw_data);

    compressed_data_length = mz_zip_compress(raw_data, raw_data_length, (char**)&compressed_data);
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

    actual_directory_record = create_zip_central_directory_record("body",
                                                                  actual_header,
                                                                  1/* zlib_stream.data_type */); /* Use 1 for now */
    memcpy(&expected_directory_record, expected_compressed_data, sizeof(expected_directory_record));
    expected_compressed_data += sizeof(expected_directory_record);

    assert_equal_zip_central_directory_record(&expected_directory_record, actual_directory_record);

    cut_assert_equal_memory("body", strlen("body"),
                            expected_compressed_data, GET_16BIT_VALUE(expected_directory_record.filename_length));

    expected_compressed_data += GET_16BIT_VALUE(expected_directory_record.filename_length);
    expected_compressed_data += GET_16BIT_VALUE(expected_directory_record.extra_field_length);

    actual_end_of_directory_record = create_zip_end_of_central_directory_record(actual_directory_record);
    memcpy(&expected_end_of_directory_record,
           expected_compressed_data,
           sizeof(expected_end_of_directory_record));
    assert_equal_zip_end_of_central_directory_record(&expected_end_of_directory_record,
                                                     actual_end_of_directory_record);
}

