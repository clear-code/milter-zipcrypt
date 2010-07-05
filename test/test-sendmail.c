/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>

#include "mz-sendmail.h"

void test_run (void);

static const char *sendmail_path;

#define MAIL_BODY "This is a test mail."
#define PASSWORD "secret"

void
setup (void)
{
    sendmail_path = cut_build_path(cut_get_test_directory(),
                                   "fixtures"
                                   "sendmail-test",
                                   "sendmail-test",
                                   NULL);
}

void
teardown (void)
{
}

void
test_send (void)
{
    cut_assert_equal_int(0,
                         mz_sendmail_send_password_mail(sendmail_path,
                                                        "from@example.com",
                                                        "to@example.com",
                                                        MAIL_BODY,
                                                        strlen(MAIL_BODY),
                                                        PASSWORD,
                                                        1000));
}

