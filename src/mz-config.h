/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_CONFIG_H__
#define __MZ_CONFIG_H__

#include <stdbool.h>

typedef struct _MzConfig MzConfig;

MzConfig     *mz_config_load   (const char *filename);
bool          mz_config_reload (MzConfig *config);
void          mz_config_free   (MzConfig *config);
const char   *mz_config_get_string
                               (MzConfig *config,
                                const char *key);
void          mz_config_set_string
                               (MzConfig *config,
                                const char *key,
                                const char *value);

#endif /* __MZ_CONFIG_H__ */

