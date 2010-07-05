#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>

#include <glib.h>

static gchar *from = NULL;

static const GOptionEntry option_entries[] =
{
    {"from", 'F', 0, G_OPTION_ARG_STRING, &from, ("Set the sender full name."), NULL},
    {NULL}
};

static gboolean
process_mail (void)
{
    GIOChannel *io;
    GIOStatus status;
    gchar *line = NULL;
    gsize length;

    io = g_io_channel_unix_new(fileno(stdin));

    do {
        status = g_io_channel_read_line(io, &line, &length, NULL, NULL);
        if (g_str_equal(line, ".\n"))
            break;
        g_print("%s\n", line);
    } while (status == G_IO_STATUS_NORMAL || status == G_IO_STATUS_AGAIN);

    g_io_channel_unref(io);

    return (status == G_IO_STATUS_NORMAL) ? TRUE : FALSE;
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

    success = process_mail();

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
