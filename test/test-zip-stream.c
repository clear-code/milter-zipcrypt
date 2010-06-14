/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>
#include <errno.h>
#include <unistd.h>

#include "mz-test-utils.h"
#include "mz-zip.h"

void test_compress (void);

static MzZipStream *zip;
static int zip_fd;
static char template[] = "MzZipStreamTestXXXXXX";

void
setup (void)
{
    cut_set_fixture_data_dir("fixtures", NULL);
    zip = mz_zip_stream_create();
}

void
teardown (void)
{
    mz_zip_stream_destroy(zip);
    if (zip_fd != -1) {
        close(zip_fd);
    }
    cut_remove_path(template, NULL);
}

void
test_compress (void)
{
#define BUFFER_SIZE 4096
    char output[BUFFER_SIZE];
    const char *raw_data;
    unsigned int raw_data_length;
    unsigned int raw_data_position = 0;
    unsigned int written_size;
    unsigned int processed_size;
    const char *expected_file;
    
    raw_data = mz_test_utils_load_data("body", &raw_data_length);
    cut_assert_not_null(raw_data);

    errno = 0;
    zip_fd = mkstemp(template);
    cut_assert_errno();

    cut_assert_true(mz_zip_stream_begin_archive(zip));

    cut_assert_true(mz_zip_stream_begin_file(zip, "body"));

    while (raw_data_position < raw_data_length) {
        cut_assert_true(mz_zip_stream_compress_step(zip,
                                                    raw_data + raw_data_position,
                                                    raw_data_length - raw_data_position,
                                                    output,
                                                    BUFFER_SIZE,
                                                    &processed_size,
                                                    &written_size));
        raw_data_position += processed_size;
        write(zip_fd, output, written_size);
    }

    cut_assert_true(mz_zip_stream_end_file(zip,
                                           output,
                                           BUFFER_SIZE,
                                           &written_size));
    write(zip_fd, output, written_size);

    cut_assert_true(mz_zip_stream_end_archive(zip,
                                              output,
                                              BUFFER_SIZE,
                                              &written_size));
    write(zip_fd, output, written_size);

    close(zip_fd);
    zip_fd = -1;

    expected_file = cut_build_path(cut_get_test_directory(),
                                   "fixtures",
                                   "stream.zip",
                                   NULL);
    cut_assert_equal_file_raw(expected_file, template);
}

