/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <crypt.h>

#include "mz-password.h"

static const char salts[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";
static const int salts_length = sizeof(salts) / sizeof(salts[0]);

static bool
get_randoms (unsigned char *randoms, size_t randoms_length)
{
    int fd;

    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
        return false;

    if (read(fd, randoms, randoms_length) != randoms_length) {
        close(fd);
        return false;
    }
    close(fd);

    return true;
}

static bool
create_salt (unsigned char *salt, size_t salt_length)
{
    unsigned char buffer[2];
    int i;

    if (!get_randoms(buffer, sizeof(buffer)))
        return false;

    for (i = 0; i < salt_length; i++)
        salt[i] = salts[buffer[i] % (salts_length - 1)];
    salt[salt_length] = '\0';

    return true;
}

char *
mz_password_create (void)
{
    unsigned char salt[3];
    struct crypt_data data;

    data.initialized = 0;
    create_salt(salt, sizeof(salt) - 1);

    return strdup(crypt_r("password", (char*)salt, &data));
}

