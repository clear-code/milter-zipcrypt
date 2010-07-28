/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_SENDMAIL_H__
#define __MZ_SENDMAIL_H__

int mz_sendmail_send_password_mail (const char   *command_path,
                                    const char   *from,
                                    const char   *recipient,
                                    const char   *body,
                                    unsigned int  body_length,
                                    const char   *boundary,
                                    const char   *password,
                                    int           timeout);

#endif /* __MZ_SENDMAIL_H__ */

