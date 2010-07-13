/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mz-config.h"

#define BUFFER_SIZE 4096
MzConfig *
mz_config_load (const char *filename)
{
    MzConfig *config;
    FILE *fp;
    char buffer[BUFFER_SIZE];

    fp = fopen(filename, "r");
    if (!fp)
        return NULL;

    config = malloc(sizeof(*config));

    while (fgets(buffer, sizeof(buffer) - 1, fp)) {
        ;
    }

    fclose(fp);

    return config;
}

bool
mz_config_reload (MzConfig *config)
{
    return true;
}

void
mz_config_free (MzConfig *config)
{
    if (config)
        free(config);
}

