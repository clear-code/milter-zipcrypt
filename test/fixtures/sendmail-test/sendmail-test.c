#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <glib.h>


#include "mz-test-sendmail-reporter.h"

static gchar *from = NULL;

static const GOptionEntry option_entries[] =
{
    {"from", 'F', 0, G_OPTION_ARG_STRING, &from, ("Set the sender full name."), NULL},
    {NULL}
};

DBusGProxy *
get_dbus_proxy (void)
{
    DBusGConnection *connection;
    DBusGProxy *proxy = NULL;

    connection = dbus_g_connection_open("unix:path=/tmp/mz-test-sendmail", NULL);
    if (connection) {
        proxy = dbus_g_proxy_new_for_peer(connection,
                                          "/org/MilterZipcrypt/Sendmail",
                                          "org.MilterZipcrypt.Sendmail");
    }
    return proxy;
}

static gboolean
process_mail (int argc, char *argv[])
{
    GIOChannel *io = NULL;
    GIOStatus status;
    gchar *line = NULL;
    gsize length;
    int i;
    DBusGProxy *proxy = NULL;
    GString *body = NULL;
    gboolean next_password = FALSE;

    proxy = get_dbus_proxy();
    /*
    if (!proxy)
        return FALSE;
    */

    if (proxy && !org_MilterZipcrypt_Sendmail_from(proxy, from, NULL))
        goto fail;

    if (proxy) {
        for (i = 1; i < argc; i++) {
            if (!org_MilterZipcrypt_Sendmail_recipient(proxy, argv[i], NULL))
                goto fail;
        }
    }

    io = g_io_channel_unix_new(STDIN_FILENO);
    g_io_channel_set_line_term(io, "\r\n", 2);
    body = g_string_new(NULL);

    do {
        status = g_io_channel_read_line(io, &line, &length, NULL, NULL);
        if (line) {
            if (next_password) {
                gchar *password;
                password = line;
                if (password && proxy && !org_MilterZipcrypt_Sendmail_password(proxy, password, NULL))
                    goto fail;
                next_password = FALSE;
            }
            if (g_str_equal(line, ".\r\n"))
                break;
            if (g_str_has_prefix(line, "The password of "))
                next_password = TRUE;
            g_string_append(body, line);
        }
    } while (status == G_IO_STATUS_NORMAL || status == G_IO_STATUS_AGAIN);

    g_io_channel_unref(io);
    io = NULL;

    if (proxy && !org_MilterZipcrypt_Sendmail_body(proxy, body->str, NULL))
        goto fail;

    g_string_free(body, TRUE);
    body = NULL;

    return (status == G_IO_STATUS_NORMAL) ? TRUE : FALSE;

fail:
    g_object_unref(proxy);
    if (body)
        g_string_free(body, TRUE);
    if (io)
        g_io_channel_unref(io);

    return FALSE;
}

int
main (int argc, char *argv[])
{
    GError *error = NULL;
    gboolean success = FALSE;
    GOptionContext *option_context;

    option_context = g_option_context_new("[RECIPIENT ADDRESS ...]");
    g_option_context_add_main_entries(option_context, option_entries, NULL);
    if (!g_option_context_parse(option_context, &argc, &argv, &error)) {
        g_print("%s\n", error->message);
        g_error_free(error);
        g_option_context_free(option_context);
        exit(EXIT_FAILURE);
    }

    g_type_init();
    success = process_mail(argc, argv);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
