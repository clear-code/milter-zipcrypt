/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#include <gcutter.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "mz-sendmail.h"

void test_run (void);

static const char *sendmail_path;
static GObject *receiver;
static DBusGConnection *g_connection;
static DBusServer *server;

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
static void
new_connection_func (DBusServer *dbus_server, DBusConnection *connection, gpointer user_data)
{
    cut_assert_not_null(receiver);

    dbus_connection_setup_with_g_main(connection, NULL);

    g_connection = dbus_connection_get_g_connection(connection);
    dbus_g_connection_ref(g_connection);
    dbus_g_connection_register_g_object(g_connection,
                                        "/org/MilterZipcrypt/Sendmail",
                                        receiver);
}

static void
setup_receiver (void)
{
    DBusError dbus_error;

    dbus_error_init(&dbus_error);
    server = dbus_server_listen("unix:path=/tmp/mz-test-sendmail", &dbus_error);
    dbus_server_setup_with_g_main(server, NULL);

    dbus_server_set_new_connection_function(server, new_connection_func, NULL, NULL);
}

void
cut_startup (void)
{
    dbus_g_thread_init();
    register_dbus_object();
    setup_receiver();
}

void
cut_shutdown (void)
{
    if (g_connection)
        dbus_g_connection_unref(g_connection);
    if (server)
        dbus_server_disconnect(server);
}

void
setup (void)
{
    sendmail_path = cut_build_path(cut_get_test_directory(),
                                   "fixtures",
                                   "sendmail-test",
                                   "sendmail-test",
                                   NULL);
    actual_from = NULL;
    n_actual_froms = 0;
    actual_recipients = NULL;
    actual_body = g_string_new(NULL);
    actual_password = NULL;
    n_actual_passwords = 0;
    receiver = g_object_new(test_dbus_object_type, NULL);
}

void
teardown (void)
{
    g_free(actual_from);
    if (actual_recipients) {
        g_list_foreach(actual_recipients, (GFunc)g_free, NULL);
        g_list_free(actual_recipients);
    }
    if (actual_body)
        g_string_free(actual_body, TRUE);
    g_free(actual_password);

    if (receiver)
        g_object_unref(receiver);
}

#define MAIL_BODY "This is a test mail."
#define PASSWORD "secret"

typedef struct _SendmailProcessStatus {
    gboolean is_timeouted;
    gboolean is_terminated;
    int exit_status;
} SendmailProcessStatus;

static gpointer
send_password_mail_thread (gpointer data)
{
    SendmailProcessStatus *status = data;

    status->exit_status = mz_sendmail_send_password_mail(sendmail_path,
                                                         "from@example.com",
                                                         "to@example.com",
                                                         MAIL_BODY,
                                                         strlen(MAIL_BODY),
                                                         PASSWORD,
                                                         3);

    status->is_terminated = TRUE;

    return NULL;
}

static gboolean
timeout_sending_password (gpointer data)
{
    SendmailProcessStatus *status = data;

    status->is_timeouted = TRUE;

    return FALSE;
}

#define EXPECTED_BODY                               \
"Subject: test\r\n"                                 \
"From: from@example.com\r\n"                        \
"To: to@example.com\r\n"                            \
"MIME-Version: 1.0\r\n"                             \
"Conent-Type: text/plain; charset=\"UTF-8\"\r\n"    \
"Conent-Transfer-Encoding: 8bit\r\n"                \
"The password of the attachment file"               \
" in the following mail is: secret\r\n"             \
"This is a test mail.\r\n"                          \

void
test_send (void)
{
    GError *error = NULL;
    GThread *thread;
    guint timeout_id;
    GString expected_body = { EXPECTED_BODY, strlen(EXPECTED_BODY) };
    GList expected_recipients = { "to@example.com", NULL };
    SendmailProcessStatus status = { FALSE, FALSE, -1 };

    thread = g_thread_create(send_password_mail_thread, &status, TRUE, &error);
    gcut_assert_error(error);

    timeout_id = g_timeout_add_seconds(3, timeout_sending_password, &status);
    while (!status.is_timeouted && !status.is_terminated)
        g_main_context_iteration(NULL, FALSE);
    g_source_remove(timeout_id);
    g_thread_join(thread);

    cut_assert_true(WIFEXITED(status.exit_status));
    cut_assert_equal_int(EXIT_SUCCESS, WEXITSTATUS(status.exit_status));
    cut_assert_equal_string("from@example.com", actual_from);
    gcut_assert_equal_list_string(&expected_recipients, actual_recipients);
    cut_assert_match("secret", actual_password);
    gcut_assert_equal_string(&expected_body, actual_body);
}

