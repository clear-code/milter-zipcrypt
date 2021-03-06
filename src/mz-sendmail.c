/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "mz-sendmail.h"

typedef enum {
    READ,
    WRITE
} _PipeMode;

void
close_pipe (int *pipe, _PipeMode mode)
{
    if (pipe[mode] == -1)
        return;
    close(pipe[mode]);
    pipe[mode] = -1;
}

static int
sane_dup2 (int fd1, int fd2)
{
    int ret;
    do
        ret = dup2(fd1, fd2);
    while (ret < 0 && errno == EINTR);
    return ret;
}

#define SUBJECT_HEADER                   "Subject: "
#define FROM_HEADER                      "From: "
#define TO_HEADER                        "To: "
#define MIME_HEADER                      "MIME-Version: 1.0"
#define CONTENT_TYPE_HEADER              "Content-Type: text/plain; charset=\"UTF-8\""
#define CONTENT_TRANSFER_ENCODING_HEADER "Content-Transfer-Encoding: 8bit"
#define SUBJECT                          "MilterZipCrypt: Password Notification"

#define CRLF "\r\n"
static size_t CRLF_LENGTH = sizeof(CRLF) - 1;

static bool
output_headers (int fd,
                const char *from,
                const char *recipient,
                const char *boundary)
{
    size_t ret;

    ret = write(fd, SUBJECT_HEADER, strlen(SUBJECT_HEADER));
    ret = write(fd, SUBJECT, strlen(SUBJECT));
    ret = write(fd, CRLF, CRLF_LENGTH);

    ret = write(fd, TO_HEADER, strlen(TO_HEADER));
    ret = write(fd, recipient, strlen(recipient));
    ret = write(fd, CRLF, CRLF_LENGTH);

    ret = write(fd, MIME_HEADER, strlen(MIME_HEADER));
    ret = write(fd, CRLF, CRLF_LENGTH);

    ret = write(fd, "Content-Type: multipart/mixed; boundary=\"",
                strlen("Content-Type: multipart/mized; boundary=\""));
    ret = write(fd, boundary, strlen(boundary));
    ret = write(fd, "\"", 1);
    ret = write(fd, CRLF, CRLF_LENGTH);

    return true;
}

#define PASSWORD_MESSAGE "The password of the attachment file in the following mail is:"

static bool
output_password (int fd, const char *password, const char *boundary)
{
    size_t ret;

    ret = write(fd, "--", 2);
    ret = write(fd, boundary, strlen(boundary));
    ret = write(fd, CRLF, CRLF_LENGTH);

    ret = write(fd, CONTENT_TYPE_HEADER, strlen(CONTENT_TYPE_HEADER));
    ret = write(fd, CRLF, CRLF_LENGTH);
    ret = write(fd, CONTENT_TRANSFER_ENCODING_HEADER, strlen(CONTENT_TRANSFER_ENCODING_HEADER));
    ret = write(fd, CRLF, CRLF_LENGTH);
    ret = write(fd, CRLF, CRLF_LENGTH);

    ret = write(fd, PASSWORD_MESSAGE, strlen(PASSWORD_MESSAGE));
    ret = write(fd, CRLF, CRLF_LENGTH);

    ret = write(fd, password, strlen(password));
    ret = write(fd, CRLF, CRLF_LENGTH);

    return true;
}

static bool
output_body (int fd, const char *body, unsigned int body_length)
{
    size_t ret;

    ret = write(fd, body, body_length);
    ret = write(fd, CRLF, CRLF_LENGTH);
    ret = write(fd, "." CRLF, CRLF_LENGTH + 1);

    return true;
}

int
mz_sendmail_send_password_mail (const char   *command_path,
                                const char   *from,
                                const char   *recipient,
                                const char   *body,
                                unsigned int  body_length,
                                const char   *boundary,
                                const char   *password,
                                int           timeout)
{
    pid_t pid;
    int stdout_pipe[2];
    int stderr_pipe[2];
    int stdin_pipe[2];

    if (pipe(stdout_pipe) < 0 ||
        pipe(stderr_pipe) < 0 ||
        pipe(stdin_pipe) < 0) {
        return -1;
    }

    pid = fork();
    if (pid == -1)
        return -1;

    if (pid == 0) {
        close_pipe(stdout_pipe, READ);
        close_pipe(stderr_pipe, READ);
        close_pipe(stdin_pipe, WRITE);

        if (sane_dup2(stdin_pipe[READ], STDIN_FILENO) < 0 ||
            sane_dup2(stdout_pipe[WRITE], STDOUT_FILENO) < 0 ||
            sane_dup2(stderr_pipe[WRITE], STDERR_FILENO) < 0) {
        }

        if (stdin_pipe[READ] >= 3)
            close_pipe(stdin_pipe, READ);
        if (stdout_pipe[WRITE] >= 3)
            close_pipe(stdout_pipe, WRITE);
        if (stderr_pipe[WRITE] >= 3)
            close_pipe(stderr_pipe, WRITE);

        execl(command_path, command_path, recipient, (char*)NULL);
        _exit(-1);
    } else {
        int status;

        close_pipe(stdout_pipe, WRITE);
        close_pipe(stderr_pipe, WRITE);
        close_pipe(stdin_pipe, READ);

        output_headers(stdin_pipe[WRITE], from, recipient, boundary);
        output_password(stdin_pipe[WRITE], password, boundary);
        output_body(stdin_pipe[WRITE], body, body_length);

        while (waitpid(pid, &status, WNOHANG) <= 0);
  
        return status;
    }

    return 0;
}


