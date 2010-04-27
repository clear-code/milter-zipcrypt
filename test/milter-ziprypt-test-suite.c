/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>
#include <dlfcn.h>
#include <stdio.h>

void mz_test_warmup (void);
void mz_test_cooldown (void);

void
mz_test_warmup (void)
{
    void *handle;
    void *func;

    handle = dlopen(NULL, RTLD_LAZY);

    func = dlsym(handle, "_header");
    cut_assert(func);
    printf("%p\n", func);
}

void
mz_test_cooldown (void)
{
}

