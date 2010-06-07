/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <cutter.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "mz-test-utils.h"

char *
mz_test_utils_load_data (const char *path, unsigned int *size)
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

