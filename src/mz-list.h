/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_LIST_H__
#define __MZ_LIST_H__

typedef struct _MzList MzList;

struct _MzList
{
    void *data;
    MzList *next;
};

typedef void (*MzElementFreeFunc)  (void *data);

#define mz_list_next(list) ((list) ? (((MzList*)(list))->next) : NULL)

MzList       *mz_list_append (MzList *list, void *data);
void          mz_list_free   (MzList *list);
void          mz_list_free_with_free_func
                             (MzList *list,
                              MzElementFreeFunc free_func);
unsigned int  mz_list_length (MzList *list);

#endif /* __MZ_LIST_H__ */
