/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gcutter.h>
#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>
#else
#  ifndef WEXITSTATUS
#    define WEXITSTATUS(status) (status)
#  endif
#endif
#include "mz-list.h"
#include "mz-utils.h"
#include "mz-test-utils.h"

void test_no_attachment (void);

static GCutProcess *test_server;
static GCutProcess *milter_zipcrypt;
static GCutProcess *unzip;
static GString *modified_message;

static char *temporary_socket;
static MzList *modified_attachments;
static int attachment_fd;
static char *attachment_file;
static const char *sendmail_path;
static const char *config_path;

void
cut_startup (void)
{
    FILE *fp;

    cut_make_directory(cut_get_test_directory(),
                       "fixtures",
                       "config",
                       NULL);

    config_path = cut_build_path(cut_get_test_directory(),
                                 "fixtures",
                                 "config",
                                 "test-milter-zipcrypt.conf",
                                 NULL);

    fp = fopen(config_path, "w+");
    sendmail_path = cut_build_path(cut_get_test_directory(),
                                   "fixtures",
                                   "sendmail-test",
                                   "sendmail-test",
                                   NULL);
    fprintf(fp, "sendmail_path = %s\n", sendmail_path);
    fclose(fp);
}

void
cut_shutdown (void)
{
    cut_remove_path(config_path, NULL);
}

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
                                        "-s", temporary_socket,
                                        "-c", config_path,
                                        NULL);
    gcut_process_run(milter_zipcrypt, &error);
    gcut_assert_error(error);
}

static void
create_temporary_socket (void)
{
    int temporary_fd;

    temporary_socket = g_strdup("unix:milter-zipcrypt-socket-XXXXXX");
    temporary_fd = mkstemp(temporary_socket);
    close(temporary_fd);
    cut_remove_path(temporary_socket, NULL);
}

void
setup (void)
{
    test_server = NULL;
    unzip = NULL;
    modified_attachments = NULL;
    attachment_fd = -1;

    attachment_file = strdup("MilterZipCryptTestXXXXXX");
    create_temporary_socket();
    cut_trace(run_milter_zipcrypt());
    cut_make_directory("tmp", NULL);
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
    if (unzip)
        g_object_unref(unzip);
    g_free(temporary_socket);
    mz_list_free(modified_attachments);

    if (attachment_fd != -1)
        close(attachment_fd);
    cut_remove_path(attachment_file, NULL);
    g_free(attachment_file);
    cut_remove_path("tmp", NULL);
}

static void
run_test_server (const char *data_file_name)
{
    GError *error = NULL;
    const char *data_path;

    data_path = cut_build_path(cut_get_source_directory(),
                               "test",
                               "fixtures",
                               data_file_name,
                               NULL);
    test_server = gcut_process_new(MILTER_TEST_SERVER,
                                   "-s", temporary_socket,
                                   "-m", data_path,
                                   "--output-message",
                                   "--color=no",
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

static void
output_attachment_to_file (MzAttachment *attachment)
{
    ssize_t written_size;

    attachment_fd = g_mkstemp(attachment_file);
    written_size = write(attachment_fd, attachment->data, attachment->data_length);
    cut_assert_equal_int(attachment->data_length, written_size);

    close(attachment_fd);
    attachment_fd = -1;
}

static const char *
get_zip_password (const char *modified_message)
{
    char *crlf_pos;
    char *password;

    password = strstr(modified_message, "X-ZIP-Crypted-Password: ");
    if (!password)
        return NULL;

    password += strlen("X-ZIP-Crypted-Password: ");
    crlf_pos = strchr(password, '\n');
    if (!crlf_pos)
        return NULL;

    return cut_take_string(strndup(password, crlf_pos - password));
}

void
test_attachment (void)
{
    GError *error = NULL;
    const char *password;
    const char *expected_file;

    cut_trace(run_test_server("test-mail"));
    cut_trace(assert_status("pass"));
    cut_assert_match("\\+ X-ZIP-Crypted: Yes", modified_message->str);

    modified_attachments = mz_utils_extract_attachments(modified_message->str,
                                                        "=-36rDyTciyqG6Meu5WLLY");
    cut_assert_not_null(modified_attachments);
    cut_assert_equal_int(1, mz_list_length(modified_attachments));

    if (!g_find_program_in_path("unzip"))
        cut_omit("unzip is not installed in your system!");

    output_attachment_to_file(modified_attachments->data);

    password = get_zip_password(modified_message->str);
    cut_assert_not_null(password);

    unzip = gcut_process_new("unzip",
                             "-o",
                             "-P", password,
                             "-d", "tmp",
                             attachment_file, NULL);
    cut_assert_true(gcut_process_run(unzip, &error));
    gcut_assert_error(error);

    cut_assert_equal_int(0, gcut_process_wait(unzip, 10000, &error));
    gcut_assert_error(error);

    cut_assert_exist_path("tmp" G_DIR_SEPARATOR_S "t.png");
    expected_file = mz_test_utils_build_fixture_data_path("t.png", NULL);
    cut_assert_equal_file_raw(expected_file,
                              "tmp" G_DIR_SEPARATOR_S "t.png");

    cut_assert_exist_path("tmp" G_DIR_SEPARATOR_S "text");
    expected_file = mz_test_utils_build_fixture_data_path("text", NULL);
    cut_assert_equal_file_raw(expected_file,
                              "tmp" G_DIR_SEPARATOR_S "text");
}

