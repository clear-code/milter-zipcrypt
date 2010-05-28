/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "mz-utils.h"

void test_get_content_type (void);
void test_get_content_transfer_encoding (void);
void test_get_content_disposition (void);
void test_get_content_disposition_with_line_feed (void);
void test_get_attachment_body_place (void);
void test_get_decoded_attachment_body (void);
void test_get_content_disposition_mime_encoded_filename (void);
void test_get_rfc2311_filename (void);
void test_parse_body (void);

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

static char *
load_data (const char *path, unsigned int *size)
{
    char *data = NULL;
    char *data_path;
    struct stat stat_buf;
    int fd;
    unsigned int bytes_read = 0;

    data_path = cut_build_fixture_data_path(path, NULL);
    cut_take_string(data_path);

    fd = open(data_path, O_RDONLY);
    if (fd < 0)
        return NULL;

    if (fstat(fd, &stat_buf) < 0)
        goto error;

    if (stat_buf.st_size <= 0 || !S_ISREG (stat_buf.st_mode))
        goto error;

    data = malloc(stat_buf.st_size);
    if (!data)
        goto error;

    *size = stat_buf.st_size;
    while (bytes_read < *size) {
        int rc;

        rc = read(fd, data + bytes_read, *size - bytes_read);
        if (rc < 0) {
            if (errno != EINTR)
                goto error;
        } else if (rc == 0) {
            break;
        } else {
            bytes_read += rc;
        }
    }

    close(fd);
    return data;

error:
    free(data);
    close(fd);

    return NULL;
}

void
test_get_content_disposition (void)
{
    char *type = NULL, *filename = NULL;
    const char *content;
    unsigned int length;

    cut_take_string(type);
    cut_take_string(filename);

    content = load_data("attachment", &length);
    cut_assert_not_null(content);

    cut_assert_true(mz_utils_get_content_disposition(content, length, &type, &filename));
    cut_assert_equal_string("attachment", type);
    cut_assert_equal_string("t.png", filename);
}

void
test_get_content_disposition_with_line_feed (void)
{
    char *type = NULL, *filename = NULL;
    const char *content;
    unsigned int length;

    cut_take_string(type);
    cut_take_string(filename);

    content = load_data("attachment_content_disposition_with_line_feed", &length);
    cut_assert_not_null(content);

    cut_assert_true(mz_utils_get_content_disposition(content, length, &type, &filename));
    cut_assert_equal_string("attachment", type);
    cut_assert_equal_string("t.png", filename);
}

void
test_get_content_disposition_mime_encoded_filename (void)
{
    char *type = NULL, *filename = NULL;
    const char *content;
    unsigned int length;

    cut_take_string(type);
    cut_take_string(filename);

    content = load_data("attachment_filename_is_mime_encoded", &length);
    cut_assert_not_null(content);

    cut_assert_true(mz_utils_get_content_disposition(content, length, &type, &filename));
    cut_assert_equal_string("attachment", type);
    cut_assert_equal_string("日本語.txt", filename);
}

void
test_get_rfc2311_filename (void)
{
    char *type = NULL, *filename = NULL;
    const char *content;
    unsigned int length;

    cut_take_string(type);
    cut_take_string(filename);

    content = load_data("rfc2311", &length);
    cut_assert_not_null(content);

    cut_assert_true(mz_utils_get_content_disposition(content, length, &type, &filename));
    cut_assert_equal_string("attachment", type);
    cut_assert_equal_string("新しいテキスト ドキュメント.txt", filename);
}

void
test_get_attachment_body_place (void)
{
    const char *content;
    const char *body;
    const char *expected_body;
    unsigned int size = 0;

    content = cut_get_fixture_data_string("attachment", NULL);
    cut_assert_not_null(content);

    expected_body = cut_get_fixture_data_string("attachment_body", NULL);
    cut_assert_not_null(expected_body);

    body = mz_utils_get_attachment_body_place(content, &size);
    cut_assert_not_null(body);
    cut_assert_not_equal_int(0, size);

    cut_assert_equal_int(strlen(expected_body), size);
    cut_assert_equal_substring(expected_body, body, size);
}

void
test_get_decoded_attachment_body (void)
{
    const char *content;
    char *expected;
    const char *body;
    unsigned int size = 0;
    unsigned int expected_size = 0;

    content = cut_get_fixture_data_string("attachment", NULL);
    cut_assert_not_null(content);

    expected = load_data("t.png", &expected_size);
    cut_take_string(expected);
    cut_assert_not_null(expected);

    body = mz_utils_get_decoded_attachment_body(content, &size);
    cut_assert_not_null(body);
    cut_assert_not_equal_int(0, size);

    cut_assert_equal_memory(expected, expected_size, body, size);
}

void
test_parse_body (void)
{
    const char *body;

    body = cut_get_fixture_data_string("body", NULL);
    cut_assert_not_null(body);

    cut_assert_true(mz_utils_parse_body(body, "=-u231oNe9VILCVd42q7nh"));
}

