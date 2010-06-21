/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_CONVERT_H__
#define __MZ_CONVERT_H__

char *mz_convert           (const char *string,
                            size_t length,
                            const char *to_charset,
                            const char *from_charset,
                            size_t *bytes_read,
                            size_t *bytes_written);

#endif /* __MZ_CONVERT_H__ */

