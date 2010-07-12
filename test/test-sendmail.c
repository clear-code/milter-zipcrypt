/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <cutter.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include "mz-sendmail.h"

void test_run (void);

static const char *sendmail_path;
static gchar *actual_from;
static guint n_actual_froms;
static GList *actual_recipients;
static GString *actual_body;
static gchar *actual_password;
static guint n_actual_passwords;

typedef struct _TestDBusObject TestDBusObject;
typedef struct _TestDBusObjectClass TestDBusObjectClass;

static gboolean
mz_test_sendmail_from (TestDBusObject *object, const gchar *from, GError **error)
{
    n_actual_froms++;

    g_free(actual_from);
    actual_from = g_strdup(from);

    return TRUE;
}

static gboolean
mz_test_sendmail_recipient (TestDBusObject *object, const gchar *recipient, GError **error)
{
    actual_recipients = g_list_append(actual_recipients, g_strdup(recipient));
    return TRUE;
}

static gboolean
mz_test_sendmail_body (TestDBusObject *object, const gchar *body, GError **error)
{
    g_string_append(actual_body, body);
    return TRUE;
}

static gboolean
mz_test_sendmail_password (TestDBusObject *object, const gchar *password, GError **error)
{
    n_actual_passwords++;

    g_free(actual_password);
    actual_password = g_strdup(password);

    return TRUE;
}

#include "mz-test-sendmail-receiver.h"

struct _TestDBusObject
{
    GObject object;
};

struct _TestDBusObjectClass
{
    GObjectClass parent_class;
};

static GType test_dbus_object_type = 0;
static GObjectClass *parent_class;

static void
class_init (GObjectClass *klass)
{
    parent_class = g_type_class_peek_parent(klass);
}

static void
init (TestDBusObject *object)
{
}

static void
register_dbus_object (void)
{
    static const GTypeInfo info =
        {
            sizeof (TestDBusObjectClass),
            (GBaseInitFunc)NULL,
            (GBaseFinalizeFunc)NULL,
            (GClassInitFunc)class_init,
            NULL,           /* class_finalize */
            NULL,           /* class_data */
            sizeof(TestDBusObject),
            0,
            (GInstanceInitFunc)init,
        };

    test_dbus_object_type = g_type_register_static(G_TYPE_OBJECT,
                                                   "TestDBusObject",
                                                   &info, 0);
    dbus_g_object_type_install_info(test_dbus_object_type, &dbus_glib_mz_test_sendmail_object_info);
}

#define MAIL_BODY "This is a test mail."
#define PASSWORD "secret"

void
cut_startup (void)
{
    register_dbus_object();
}

void
setup (void)
{
    sendmail_path = cut_build_path(cut_get_test_directory(),
                                   "fixtures",
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
    int status;

    status = mz_sendmail_send_password_mail(sendmail_path,
                                            "from@example.com",
                                            "to@example.com",
                                            MAIL_BODY,
                                            strlen(MAIL_BODY),
                                            PASSWORD,
                                            3000);
    cut_assert_true(WIFEXITED(status));
    cut_assert_equal_int(EXIT_SUCCESS, WEXITSTATUS(status));
}

