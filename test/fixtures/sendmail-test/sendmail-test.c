#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <glib.h>
#include <dbus/dbus-glib.h>

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
                                          "org/MilterZipcrypt/Sendmail",
                                          "org.MilterZipcrypt.Sendmail");
    }
    return proxy;
}

static gboolean
process_mail (void)
{
    GIOChannel *io = NULL;
    GIOStatus status;
    gchar *line = NULL;
    gsize length;
    DBusGProxy *proxy = NULL;
    GString *body = NULL;

    proxy = get_dbus_proxy();
    /*
    if (!proxy)
        return FALSE;
    */

    if (proxy && !org_MilterZipcrypt_Sendmail_from(proxy, from, NULL))
        goto fail;

    io = g_io_channel_unix_new(STDIN_FILENO);
    body = g_string_new(NULL);

    do {
        status = g_io_channel_read_line(io, &line, &length, NULL, NULL);
        if (g_str_equal(line, ".\n"))
            break;
        g_string_append(body, line);
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

    option_context = g_option_context_new(NULL);
    g_option_context_add_main_entries(option_context, option_entries, NULL);
    if (!g_option_context_parse(option_context, &argc, &argv, &error)) {
        g_print("%s\n", error->message);
        g_error_free(error);
        g_option_context_free(option_context);
        exit(EXIT_FAILURE);
    }

    g_type_init();
    success = process_mail();

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
