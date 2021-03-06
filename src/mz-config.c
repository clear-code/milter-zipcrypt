/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "mz-config.h"

#include "mz-list.h"

struct _MzConfig
{
    MzList *key_value_pairs;
    char *file_path;
};

typedef struct _KeyValuePair KeyValuePair;
struct _KeyValuePair
{
    char *key;
    char *value;
};

static void
chomp_string (char *string)
{
    char *p;
    for (p = string; *p != '\0'; p++) {
        if (*p == '#' || *p == '\n')
            *p = '\0';
    }
}

static void
add_key (MzConfig *config, char *key, char *value)
{
    KeyValuePair *pair;

    pair = malloc(sizeof(*pair));
    pair->key = key;
    pair->value = value;

    config->key_value_pairs = mz_list_append(config->key_value_pairs, pair);
}

static MzConfig *
mz_config_new (const char *filename)
{
    MzConfig *config;

    config = malloc(sizeof(*config));
    config->key_value_pairs = NULL;
    config->file_path = strdup(filename);

    return config;
}

#define BUFFER_SIZE 4096
static void
_load_from_file (MzConfig *config)
{
    FILE *fp;
    char buffer[BUFFER_SIZE];

    fp = fopen(config->file_path, "r");
    if (!fp)
        return;

    while (fgets(buffer, sizeof(buffer) - 1, fp)) {
        char *beginning = buffer;
        char *key_end;
        char *value_start;
        char *key, *value;

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

        key = strndup(beginning, key_end - beginning + 1),
        value = strdup(value_start);
        mz_config_set_string(config, key, value);
        free(key);
        free(value);
    }

    fclose(fp);
}

MzConfig *
mz_config_load (const char *filename)
{
    MzConfig *config;

    config = mz_config_new(filename);

    _load_from_file(config);

    return config;
}

bool
mz_config_reload (MzConfig *config)
{
    _load_from_file(config);
    return true;
}

static void
key_value_pair_free (KeyValuePair *pair)
{
    if (!pair)
        return;

    if (pair->key)
        free(pair->key);
    if (pair->value)
        free(pair->value);
    free(pair);
}

void
mz_config_free (MzConfig *config)
{
    if (!config)
        return;

    if (config->key_value_pairs) {
        mz_list_free_with_free_func(config->key_value_pairs,
                                    (MzListElementFreeFunc)key_value_pair_free);
    }

    free(config->file_path);
    free(config);
}

static bool
key_value_pair_key_equal (KeyValuePair *a, KeyValuePair *b)
{
    if (!a || !b)
        return false;

    return (strcmp(a->key, b->key) == 0);
}

const char *
mz_config_get_string (MzConfig *config, const char *key)
{
    KeyValuePair pair;
    MzList *found;

    if (!config || !config->key_value_pairs)
        return NULL;

    pair.key = (char*)key;
    pair.value = NULL;

    found = mz_list_find_with_equal_func(config->key_value_pairs,
                                         &pair,
                                         (MzListElementEqualFunc)key_value_pair_key_equal);
    if (!found)
        return NULL;

    return ((KeyValuePair*)(found->data))->value;
}

void
mz_config_set_string (MzConfig *config, const char *key, const char *value)
{
    KeyValuePair pair;
    MzList *found;

    if (!config)
        return;

    pair.key = (char*)key;
    pair.value = NULL;

    found = mz_list_find_with_equal_func(config->key_value_pairs,
                                         &pair,
                                         (MzListElementEqualFunc)key_value_pair_key_equal);
    if (!found) {
        add_key(config, strdup(key), strdup(value));
    } else {
        KeyValuePair *old_pair = (KeyValuePair*)found->data;
        if (old_pair->value)
            free(old_pair->value);
        old_pair->value = strdup(value);
    }
}

