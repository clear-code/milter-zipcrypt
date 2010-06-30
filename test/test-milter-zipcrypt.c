/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gcutter.h>
#include <unistd.h>
#ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>
#else
#  define WEXITSTATUS(status) (status)
#endif

void test_run (void);

static GCutProcess *test_server;
static GCutProcess *milter_zipcrypt;

#define CONNECTION_SPEC "unix:test-milter-zipcrypt.sock"
static char *temporary_socket;

static void
run_milter_zipcrypt (void)
{
    GError *error = NULL;
    const gchar *milter_zipcrypt_path;

    milter_zipcrypt_path = cut_build_path(cut_get_test_directory(),
                                          "..",
                                          "src",
                                          "milter-zipcrypt",
                                          NULL);
    milter_zipcrypt = gcut_process_new(milter_zipcrypt_path,
                                        "-s",
                                        temporary_socket,
                                        NULL);
    gcut_process_run(milter_zipcrypt, &error);
    gcut_assert_error(error);
}

static void
create_temporary_socket (void)
{
    int fd;

    temporary_socket = g_strdup("unix:milter-zipcrypt-socket-XXXXXX");
    fd = mkstemp(temporary_socket);
    close(fd);
    cut_remove_path(temporary_socket, NULL);
}

void
setup (void)
{
    test_server = NULL;

    create_temporary_socket();
    cut_trace(run_milter_zipcrypt());
}

static void
wait_reaped_helper (GCutProcess *process)
{
    GError *error = NULL;
    gint exit_status;

    exit_status = gcut_process_wait(process, 1000, &error);
    gcut_assert_error(error);

    cut_assert_equal_int(EXIT_SUCCESS, WEXITSTATUS(exit_status));
}

#define wait_reaped(process)                  \
    cut_trace(wait_reaped_helper(process))

void
teardown (void)
{
    if (milter_zipcrypt)
        g_object_unref(milter_zipcrypt);
    if (test_server)
        g_object_unref(test_server);
    g_free(temporary_socket);
}

static void
run_test_server (void)
{
    GError *error = NULL;

    test_server = gcut_process_new(MILTER_TEST_SERVER,
                                   "-s",
                                   temporary_socket,
                                   NULL);
    gcut_process_run(test_server, &error);
    gcut_assert_error(error);
    wait_reaped(test_server);
}

void
test_run (void)
{
    cut_trace(run_test_server());
}

