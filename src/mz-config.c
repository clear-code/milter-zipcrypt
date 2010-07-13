/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "mz-config.h"

static void
chomp_string (char *string)
{
    char *p;
    for (p = string; *p != '\0'; p++) {
        if (*p == '#' || *p == '\n')
            *p = '\0';
    }
}

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

    memset(buffer, 0, BUFFER_SIZE);
    config = malloc(sizeof(*config));

    while (fgets(buffer, sizeof(buffer) - 1, fp)) {
        char *beginning = buffer;
        char *key_end;
        char *value_start;

        while (isspace(*beginning))
            beginning++;

        if (*beginning == '#')
            continue;

        if ((key_end = value_start = strchr(beginning, '=')) == NULL)
            continue;

        if (key_end == beginning) /* no key */
            continue;

        key_end--;
        while (isspace(*key_end))
            key_end--;

        value_start++;
        while (isspace(*value_start))
            value_start++;
        chomp_string(value_start);

        if (!strncmp(beginning, "sendmail_path", key_end - beginning))
            config->sendmail_path = strdup(value_start);
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

