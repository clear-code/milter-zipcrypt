/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_LIST_H__
#define __MZ_LIST_H__

#include <stdbool.h>

typedef struct _MzList MzList;

struct _MzList
{
    void *data;
    MzList *next;
};

typedef void (*MzListElementFreeFunc)     (void *data);
typedef bool (*MzListElementEqualFunc)    (void *a, void *b);

#define mz_list_next(list) ((list) ? (((MzList*)(list))->next) : NULL)

MzList       *mz_list_append (MzList *list, void *data);
void          mz_list_free   (MzList *list);
void          mz_list_free_with_free_func
                             (MzList *list,
                              MzListElementFreeFunc free_func);
unsigned int  mz_list_length (MzList *list);
MzList       *mz_list_find_with_equal_func
                             (MzList *list, void *data,
                              MzListElementEqualFunc equal_func);

#endif /* __MZ_LIST_H__ */

