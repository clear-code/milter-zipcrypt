/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <string.h>
#include <stdlib.h>
#include "mz-list.h"

static MzList *
mz_list_last (MzList *list)
{
    if (list) {
        while (list->next)
            list = list->next;
    }

    return list;
}

MzList *
mz_list_append (MzList *list, void *data)
{
    MzList *new;
    MzList *last;

    new = malloc(sizeof(*new));
    if (!new)
        return NULL;

    new->data = data;
    new->next = NULL;

    if (list) {
        last = mz_list_last(list);
        last->next = new;
        return list;
    } else {
        return new;
    }
}

void
mz_list_free (MzList *list)
{
    while (list) {
        MzList *next;
        next = list->next;
        free(list);
        list = next;
    }
}

void
mz_list_free_with_free_func (MzList *list, MzListElementFreeFunc free_func)
{
    while (list) {
        MzList *next;
        next = list->next;
        free_func(list->data);
        free(list);
        list = next;
    }
}

unsigned int
mz_list_length (MzList *list)
{
    unsigned int length = 0;

    while (list) {
        length++;
        list = list->next;
    }

    return length;
}
