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
#include "mz-list.h"
#include "mz-utils.h"

void test_no_attachment (void);

static GCutProcess *test_server;
static GCutProcess *milter_zipcrypt;
static GString *modified_message;

#define CONNECTION_SPEC "unix:test-milter-zipcrypt.sock"
static char *temporary_socket;
static MzList *modified_attachments;

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
    modified_attachments = NULL;

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
    mz_list_free(modified_attachments);
}

static void
run_test_server (const char *data_file_name)
{
    GError *error = NULL;
    const char *data_path;

    data_path = cut_build_path(cut_get_test_directory(),
                               "fixtures",
                               data_file_name,
                               NULL);
    test_server = gcut_process_new(MILTER_TEST_SERVER,
                                   "-s",
                                   temporary_socket,
                                   "-m",
                                   data_path,
                                   "--output-message",
                                   NULL);
    gcut_process_run(test_server, &error);
    gcut_assert_error(error);
    wait_reaped(test_server);
    modified_message = gcut_process_get_output_string(test_server);

    cut_assert_not_null(modified_message);
}

static void
assert_status (const char *status)
{
    cut_assert_match(cut_take_printf("^status: %s\n", status),
                     modified_message->str);
}

void
test_no_attachment (void)
{
    cut_trace(run_test_server("no_attachment"));
    cut_trace(assert_status("accept"));
}

void
test_attachment (void)
{
    cut_trace(run_test_server("mail"));
    cut_trace(assert_status("pass"));
    cut_assert_match("\\+ X-ZIP-Crypted: Yes", modified_message->str);

    modified_attachments = mz_utils_extract_attachments(modified_message->str,
                                                        "=-HY8vxMUek5fQZa/zkovp");
    cut_assert_not_null(modified_attachments);
    cut_assert_equal_int(1, mz_list_length(modified_attachments));
}

