/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <cutter.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "mz-test-utils.h"

const char *
mz_test_utils_load_data (const char *path, unsigned int *size)
{
    char *data = NULL;
    char *data_path;
    struct stat stat_buf;
    int fd;
    unsigned int bytes_read = 0;

    data_path = cut_build_fixture_data_path(path, NULL);
    cut_take_string(data_path);

    if (stat(data_path, &stat_buf) < 0)
        return NULL;

    if (stat_buf.st_size <= 0 || !S_ISREG (stat_buf.st_mode))
        return NULL;

    fd = open(data_path, O_RDONLY);
    if (fd < 0)
        return NULL;

    data = malloc(stat_buf.st_size);
    if (!data)
        goto error;

    cut_take_memory(data);
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
    close(fd);

    return NULL;
}

const char *
mz_test_utils_build_fixture_data_path (const char *filename)
{
    return cut_build_path(cut_get_source_directory(),
                          "test",
                          "fixtures",
                          filename,
                          NULL);
}

time_t
mz_test_utils_get_last_modified_time (const char *path)
{
    char *data_path;
    struct stat stat_buf;

    data_path = cut_build_fixture_data_path(path, NULL);
    cut_take_string(data_path);

    if (stat(data_path, &stat_buf) < 0)
        return 0;

    return stat_buf.st_mtime;
}

unsigned int
mz_test_utils_get_file_attributes (const char *path)
{
    char *data_path;
    struct stat stat_buf;

    data_path = cut_build_fixture_data_path(path, NULL);
    cut_take_string(data_path);

    if (stat(data_path, &stat_buf) < 0)
        return 0;

    return (stat_buf.st_mode << 16) | !(stat_buf.st_mode & S_IWRITE);
}

