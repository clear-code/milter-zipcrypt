/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_FILTER_H__
#define __MZ_FILTER_H__

#include <libmilter/mfapi.h>

sfsistat mz_envfrom (SMFICTX *context, char **froms);
sfsistat mz_envrcpt (SMFICTX *context, char **recipients);
sfsistat mz_header  (SMFICTX *context, char *name, char *value);
sfsistat mz_body    (SMFICTX *context, unsigned char *chunk, size_t size);
sfsistat mz_eom     (SMFICTX *context);
sfsistat mz_abort   (SMFICTX *context);
sfsistat mz_close   (SMFICTX *context);
sfsistat mz_cleanup (SMFICTX *context);

#endif /* __MZ_FILTER_H__ */
