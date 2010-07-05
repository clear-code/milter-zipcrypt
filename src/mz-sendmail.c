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

int
mz_sendmail_send_password_mail (const char   *command_path,
                                const char   *from,
                                const char   *recipient,
                                const char   *body,
                                unsigned int  body_length,
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

        if (sane_dup2(stdout_pipe[WRITE], STDOUT_FILENO) < 0 ||
            sane_dup2(stderr_pipe[WRITE], STDERR_FILENO) < 0) {
        }

        if (stdout_pipe[WRITE] >= 3)
            close_pipe(stdout_pipe, WRITE);
        if (stderr_pipe[WRITE] >= 3)
            close_pipe(stderr_pipe, WRITE);
        execl(command_path, "-F", from, NULL);
        _exit(-1);
    } else {
        int status;
        int ret;

        close_pipe(stdout_pipe, WRITE);
        close_pipe(stderr_pipe, WRITE);
        close_pipe(stdin_pipe, READ);

        ret = waitpid(pid, &status, 0);
        if (ret < 0) {
        }
  
        return status;
    }

    return 0;
}


