/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <gcutter.h>
#include <errno.h>
#include <unistd.h>

#include "mz-test-utils.h"
#include "mz-zip.h"

void test_encrypt (void);

static MzZipStream *zip;
static int zip_fd;
static char *template;
static GCutProcess *process;

void
setup (void)
{
    zip = NULL;
    process = NULL;
    cut_set_fixture_data_dir(cut_get_source_directory(),
                             "test",
                             "fixtures",
                             NULL);
    template = strdup("MzZipStreamTestXXXXXX");
    cut_make_directory("tmp", NULL);
}

void
teardown (void)
{
    if (zip)
        mz_zip_stream_destroy(zip);
    if (zip_fd != -1)
        close(zip_fd);
    cut_remove_path(template, NULL);
    free(template);
    cut_remove_path("tmp", NULL);
    if (process)
        g_object_unref(process);
}

static void
assert_success (MzZipStreamStatus status)
{
    cut_assert_equal_int(MZ_ZIP_STREAM_STATUS_SUCCESS, status);
}

#define BUFFER_SIZE 1096
static void
compress_file (const char *filename)
{
    unsigned char output[BUFFER_SIZE];
    const char *raw_data;
    unsigned int raw_data_length;
    unsigned int raw_data_position = 0;
    unsigned int processed_size;
    unsigned int written_size;
    ssize_t ret;
    MzZipStreamStatus status = MZ_ZIP_STREAM_STATUS_SUCCESS;

    raw_data = mz_test_utils_load_data(filename, &raw_data_length);
    cut_assert_not_null(raw_data);

    assert_success(mz_zip_stream_begin_file(zip,
                                            filename,
                                            output,
                                            BUFFER_SIZE,
                                            &written_size));
    ret = write(zip_fd, output, written_size);

    do {
        status = mz_zip_stream_process_file_data(zip,
                                                 raw_data + raw_data_position,
                                                 raw_data_length - raw_data_position,
                                                 output,
                                                 BUFFER_SIZE,
                                                 &processed_size,
                                                 &written_size);
        raw_data_position += processed_size;
        ret = write(zip_fd, output, written_size);
    } while (raw_data_position < raw_data_length || written_size == BUFFER_SIZE);

    assert_success(mz_zip_stream_end_file(zip,
                                          output,
                                          BUFFER_SIZE,
                                          &written_size));
    ret = write(zip_fd, output, written_size);
}

void
test_encrypt (void)
{
    unsigned char output[BUFFER_SIZE];
    unsigned int written_size;
    const char *expected_file;
    ssize_t ret;
    GError *error = NULL;

    if (!g_find_program_in_path("unzip"))
        cut_omit("unzip is not installed in your system!");

    zip = mz_zip_stream_create("password");
    cut_assert_not_null(zip);

    errno = 0;
    zip_fd = g_mkstemp(template);
    cut_assert_errno();
    assert_success(mz_zip_stream_begin_archive(zip));

    cut_trace(compress_file("body"));
    cut_trace(compress_file("t.png"));

    assert_success(mz_zip_stream_end_archive(zip,
                                             output,
                                             BUFFER_SIZE,
                                             &written_size));
    ret = write(zip_fd, output, written_size);

    close(zip_fd);
    zip_fd = -1;

    process = gcut_process_new("unzip",
                               "-o",
                               "-P", "password",
                               "-d", "tmp",
                               template, NULL);
    cut_assert_true(gcut_process_run(process, &error));
    gcut_assert_error(error);

    cut_assert_equal_int(0, gcut_process_wait(process, 1000, &error));
    gcut_assert_error(error);

    cut_assert_exist_path("tmp" G_DIR_SEPARATOR_S "body");
    expected_file = mz_test_utils_build_fixture_data_path("body", NULL);
    cut_assert_equal_file_raw(expected_file,
                              "tmp" G_DIR_SEPARATOR_S "body");

    cut_assert_exist_path("tmp" G_DIR_SEPARATOR_S "t.png");
    expected_file = mz_test_utils_build_fixture_data_path("t.png", NULL);
    cut_assert_equal_file_raw(expected_file,
                              "tmp" G_DIR_SEPARATOR_S "t.png");
}

