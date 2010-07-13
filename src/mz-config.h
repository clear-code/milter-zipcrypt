/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_CONFIG_H__
#define __MZ_CONFIG_H__

#include <stdbool.h>

typedef struct _MzConfig MzConfig;
struct _MzConfig
{
    char *sendmail_path;
    char *charset_in_zip;
};

MzConfig     *mz_config_load   (const char *filename);
bool          mz_config_reload (MzConfig *config);
void          mz_config_free   (MzConfig *config);

#endif /* __MZ_CONFIG_H__ */

