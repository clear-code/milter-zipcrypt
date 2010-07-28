/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>
#include <getopt.h>
#include <libmilter/mfapi.h>

#include "mz-base64.h"
#include "mz-utils.h"
#include "mz-zip.h"
#include "mz-convert.h"
#include "mz-password.h"
#include "mz-config.h"
#include "mz-sendmail.h"
#include "mz-mime-utils.h"

struct MzPriv {
    char *boundary;
    char *from;
    unsigned char *body;
    size_t body_length;
    char *password;
};

static MzConfig *config;

static sfsistat _envfrom (SMFICTX *context, char **froms);
static sfsistat _envrcpt (SMFICTX *context, char **recipients);
static sfsistat _header  (SMFICTX *context, char *name, char *value);
static sfsistat _body    (SMFICTX *context, unsigned char *chunk, size_t size);
static sfsistat _eom     (SMFICTX *context);
static sfsistat _abort   (SMFICTX *context);
static sfsistat _close   (SMFICTX *context);
static sfsistat _cleanup (SMFICTX *context);

static sfsistat
_negotiate (SMFICTX *context,
            unsigned long f0,
            unsigned long f1,
            unsigned long f2,
            unsigned long f3,
            unsigned long *pf0,
            unsigned long *pf1,
            unsigned long *pf2,
            unsigned long *pf3)
{
    *pf0 = (SMFIF_ADDHDRS | SMFIF_CHGBODY);
    *pf1 = f1 & (SMFIP_NOHELO | SMFIP_NOUNKNOWN | SMFIP_NODATA | SMFIP_NOCONNECT | SMFIP_NOEOH);
    *pf2 = 0;
    *pf3 = 0;

    return SMFIS_CONTINUE;
}

static sfsistat
_envfrom (SMFICTX *context, char **froms)
{
    struct MzPriv *priv;
    priv = malloc(sizeof(*priv));

    if (!priv)
        return SMFIS_TEMPFAIL;

    memset(priv, 0, sizeof(*priv));
    smfi_setpriv(context, priv);

    priv->from = strdup(froms[0]);
    if (!priv->from)
        return SMFIS_TEMPFAIL;

    return SMFIS_CONTINUE;
}

static sfsistat
_envrcpt (SMFICTX *context, char **recipients)
{
    return SMFIS_CONTINUE;
}

static char *
get_boundary (const char *value)
{
    char *start, *end;

    start = strstr(value, "boundary=\"");
    if (!start)
        return NULL;

    start += sizeof("boundary=\"") - 1;
    end = strchr(start, '"');
    if (!end)
        return NULL;

    return strndup(start, end - start);
}

static sfsistat
_header (SMFICTX *context, char *name, char *value)
{
    struct MzPriv *priv;
    char *boundary;

    if (!strcmp(name, "X-ZIP-Crypted"))
        return SMFIS_CONTINUE;

    if (strcasecmp(name, "Content-type"))
        return SMFIS_CONTINUE;

    if (!strstr(value, "multipart/mixed;"))
        return SMFIS_CONTINUE;

    boundary = get_boundary(value);
    if (!boundary)
        return SMFIS_ACCEPT;

    priv = (struct MzPriv*)smfi_getpriv(context);
    if (!priv)
        return SMFIS_ACCEPT;

    priv->boundary = boundary;

    return SMFIS_CONTINUE;
}

static sfsistat
append_body (struct MzPriv *priv, unsigned char *chunk, size_t size)
{
    if (!priv->body) {
        priv->body = malloc(size);
        if (!priv->body)
            return SMFIS_TEMPFAIL;
        memcpy(priv->body, chunk, size);
        priv->body_length = size;
    } else {
        priv->body = realloc(priv->body, priv->body_length + size);
        if (!priv->body)
            return SMFIS_TEMPFAIL;
        memcpy(&priv->body[priv->body_length], chunk, size);
        priv->body_length += size;
    }
    return SMFIS_CONTINUE;
}

static sfsistat
_body (SMFICTX *context, unsigned char *chunk, size_t size)
{
    struct MzPriv *priv;

    priv = (struct MzPriv*)smfi_getpriv(context);
    if (!priv)
        return SMFIS_ACCEPT;

    return append_body(priv, chunk, size);
}

#define ZIP_BUFFER_SIZE 4096
#define BASE64_BUFFER_SIZE 6042 /* (ZIP_BUFFER_SIZE / 3 + 1) * 4 + 1  + 576(CRLF) */
static sfsistat
_replace_body_with_base64 (SMFICTX *context, const unsigned char *body, size_t size, int *state, int *save)
{
    unsigned int base64_length;

    unsigned char base64_output[BASE64_BUFFER_SIZE];

    base64_length = mz_base64_encode_step(body,
                                          size,
                                          1,
                                          (char*)base64_output,
                                          state,
                                          save);
    return smfi_replacebody(context, base64_output, base64_length);
}

static sfsistat
_replace_body_with_base64_close (SMFICTX *context, int *state, int *save)
{
    unsigned int base64_length;

    unsigned char base64_output[BASE64_BUFFER_SIZE];

    base64_length = mz_base64_encode_close(1,
                                          (char*)base64_output,
                                          state,
                                          save);
    return smfi_replacebody(context, base64_output, base64_length);
}

static sfsistat
_send_headers (SMFICTX *context, MzList *attachments)
{
    char *content_type = NULL;
    char *content_disposition = NULL;
    char *new_filename;
    char *encoded_filename;
    char *charset;
    int bytes_written;
    MzAttachment *attachment = attachments->data;

    new_filename = mz_utils_get_filename_without_extension(attachment->filename);
    if (!new_filename)
        new_filename = strdup("attachment");
    encoded_filename = mz_mime_utils_encode(new_filename);
    free(new_filename);

    charset = attachment->charset ? attachment->charset : "iso-8859-1";

    bytes_written = asprintf(&content_type,
                             "Content-Type: application/zip; name*=%s''%s.zip\r\n",
                             charset,
                             encoded_filename);
    bytes_written = asprintf(&content_disposition,
                             "Content-Disposition: attachment; filename*=%s''%s.zip\r\n",
                             charset,
                             encoded_filename);
    free(encoded_filename);
    smfi_replacebody(context,
                     (unsigned char*)content_type,
                     strlen(content_type));
    smfi_replacebody(context,
                     (unsigned char*)content_disposition,
                     strlen(content_disposition));
    free(content_type);
    free(content_disposition);
    smfi_replacebody(context,
                     (unsigned char*)"Content-Transfer-Encoding: base64\r\n",
                     strlen("Content-Transfer-Encoding: base64\r\n"));

    smfi_replacebody(context,
                     (unsigned char*)"\r\n",
                     strlen("\r\n"));

    return SMFIS_CONTINUE;
}

static sfsistat
_replace_with_crypted_data (SMFICTX *context, struct MzPriv *priv, MzList *attachments)
{
    MzZipStream *zip;
    MzList *node;
    unsigned char zip_output[ZIP_BUFFER_SIZE];
    unsigned int written_size;
    const char *preferred_charset;
    int state = 0;
    int save = 0;

    _send_headers(context, attachments);

    preferred_charset = mz_config_get_string(config, "charset_in_zip_file");

    zip = mz_zip_stream_create(priv->password);
    mz_zip_stream_begin_archive(zip);
    for (node = attachments; node; node = mz_list_next(node)) {
        MzAttachment *attachment = node->data;
        unsigned int zip_data_position = 0;
        char *convert_filename = NULL;

        if (preferred_charset && attachment->charset &&
            strcasecmp(preferred_charset, attachment->charset)) {
            size_t bytes_read, bytes_written;
            convert_filename = mz_convert(attachment->filename,
                                          strlen(attachment->filename),
                                          preferred_charset,
                                          attachment->charset,
                                          &bytes_read,
                                          &bytes_written);
        }

        if (mz_zip_stream_begin_file(zip,
                                     convert_filename ? convert_filename : attachment->filename,
                                     zip_output,
                                     ZIP_BUFFER_SIZE,
                                     &written_size) != MZ_ZIP_STREAM_STATUS_SUCCESS) {
            if (convert_filename)
                free(convert_filename);
            goto fail;
        }
        if (convert_filename)
            free(convert_filename);

        _replace_body_with_base64(context, zip_output, written_size, &state, &save);

        do {
            MzZipStreamStatus status;
            unsigned int processed_size;

            status = mz_zip_stream_process_file_data(zip,
                                                     attachment->data + zip_data_position,
                                                     attachment->data_length - zip_data_position,
                                                     zip_output,
                                                     ZIP_BUFFER_SIZE,
                                                     &processed_size,
                                                     &written_size);
            if (status != MZ_ZIP_STREAM_STATUS_SUCCESS &&
                status != MZ_ZIP_STREAM_STATUS_REMAIN_OUTPUT_DATA) {
                goto fail;
            }
            zip_data_position += processed_size;
            _replace_body_with_base64(context, zip_output, written_size, &state, &save);
        } while (zip_data_position < attachment->data_length || written_size == ZIP_BUFFER_SIZE);

        if (mz_zip_stream_end_file(zip,
                                   zip_output,
                                   ZIP_BUFFER_SIZE,
                                   &written_size) != MZ_ZIP_STREAM_STATUS_SUCCESS) {
            goto fail;
        }
        _replace_body_with_base64(context, zip_output, written_size, &state, &save);
    }

    if (mz_zip_stream_end_archive(zip,
                                  zip_output,
                                  ZIP_BUFFER_SIZE,
                                  &written_size) != MZ_ZIP_STREAM_STATUS_SUCCESS) {
        goto fail;
    }
    _replace_body_with_base64(context, zip_output, written_size, &state, &save);
    _replace_body_with_base64_close(context, &state, &save);

    mz_zip_stream_destroy(zip);

    smfi_replacebody(context, (unsigned char*)"\r\n", strlen("\r\n"));
    smfi_replacebody(context, (unsigned char*)"--", strlen("--"));
    smfi_replacebody(context, (unsigned char*)priv->boundary, strlen(priv->boundary));
    smfi_replacebody(context, (unsigned char*)"--", strlen("--"));
    smfi_replacebody(context, (unsigned char*)"\r\n", strlen("\r\n"));
    smfi_replacebody(context, (unsigned char*)"\r\n", strlen("\r\n"));

    return SMFIS_CONTINUE;

fail:

    mz_zip_stream_destroy(zip);

    return SMFIS_TEMPFAIL;
}

static sfsistat
_send_body (SMFICTX *context, struct MzPriv *priv, MzList *attachments)
{
    MzAttachment *body = attachments->data;
    return smfi_replacebody(context,
                            priv->body,
                            ((unsigned char*)body->boundary_start_position - priv->body));
}

static void
_set_password (struct MzPriv *priv)
{
    priv->password = mz_password_create();
}

static bool
_send_password (struct MzPriv *priv, MzList *attachments)
{
    MzAttachment *body = attachments->data;
    const char *sendmail_path;

    sendmail_path = mz_config_get_string(config, "sendmail_path");
    if (!sendmail_path) {
        syslog(LOG_ERR, "'send_mail_path' is not set in config file.");
        return false;
    }

    mz_sendmail_send_password_mail(sendmail_path,
                                   NULL,
                                   priv->from,
                                   (const char*)priv->body,
                                   body->boundary_start_position - (const char*)priv->body,
                                   priv->password,
                                   1000);
    return true;
}

static sfsistat
_eom (SMFICTX *context)
{
    struct MzPriv *priv;
    MzList *attachments;
    sfsistat ret;

    priv = (struct MzPriv*)smfi_getpriv(context);
    if (!priv)
        return SMFIS_ACCEPT;

    if (!priv->boundary)
        return SMFIS_ACCEPT;

    if (!priv->body)
        return SMFIS_ACCEPT;

    attachments = mz_utils_extract_attachments((char*)priv->body, priv->boundary);
    if (!attachments)
        return SMFIS_CONTINUE;

    _set_password(priv);

    smfi_addheader(context, "X-ZIP-Crypted", "Yes");

    _send_body(context, priv, attachments);
    ret = _replace_with_crypted_data(context, priv, attachments);

    if (ret == SMFIS_CONTINUE && !_send_password(priv, attachments))
        ret = SMFIS_TEMPFAIL;

    mz_list_free_with_free_func(attachments, (MzListElementFreeFunc)mz_attachment_free);

    return ret;
}

static sfsistat
_abort (SMFICTX *context)
{
    return _cleanup(context);
}

static sfsistat
_close (SMFICTX *context)
{
    return _cleanup(context);
}

static sfsistat
_cleanup (SMFICTX *context)
{
    struct MzPriv *priv;
    
    priv = (struct MzPriv*)smfi_getpriv(context);
    if (!priv)
        return SMFIS_CONTINUE;

    if (priv->from) {
        free(priv->from);
        priv->from = NULL;
    }

    if (priv->boundary) {
        free(priv->boundary);
        priv->boundary = NULL;
    }

    if (priv->body) {
        free(priv->body);
        priv->body = NULL;
    }

    if (priv->password) {
        free(priv->password);
        priv->password = NULL;
    }

    free(priv);
    priv = NULL;

    smfi_setpriv(context, NULL);

    return SMFIS_CONTINUE;
}

struct smfiDesc smfilter = {
    "milter-zipcrypt",
    SMFI_VERSION,
    SMFIF_CHGHDRS | SMFIF_ADDHDRS | SMFIF_CHGBODY,
    NULL, /* connect */
    NULL, /* helo */
    _envfrom,
    _envrcpt,
    _header,
    NULL, /* eoh */
    _body,
    _eom,
    _abort,
    _close,
    NULL, /* unknown */
    NULL, /* data */
    _negotiate  /* negotiate */
};

static bool
switch_user (const char *user_name)
{
    struct passwd *password;

    errno = 0;
    password = getpwnam(user_name);
    if (!password) {
        if (errno == 0)
            syslog(LOG_NOTICE, "There is no %s user.", user_name);
        else
            syslog(LOG_NOTICE, "Failed to get password for %s.", user_name);
        return false;
    }

    if (setuid(password->pw_uid) == -1) {
        syslog(LOG_NOTICE, "Failed to switch to %s.", user_name);
        return false;
    }

    return true;
}

static void
show_usage (void)
{
    printf("usage:\n"
           " %s [options]\n"
           " -d, --daemon     \t\trun as daemon process\n"
           " -c, --config-file\t\tspecify configuration file\n"
           " -p, --pid-file   \t\tchange to specified user\n"
           " -s, --spec       \t\tspecify the socket address to connect MTA\n"
           " -u, --user-name  \t\tchange to specified user\n"
           " -v, --verbose    \t\trun verbose mode\n"
           " -h, --help       \t\tshow this help message\n",
           PACKAGE_NAME);
    exit(EXIT_SUCCESS);
}

static struct option long_options[] = {
    {"daemon",      0, 0, 'd'},
    {"help",        0, 0, 'h'},
    {"config-file", 1, 0, 'c'},
    {"pid-file",    1, 0, 'p'},
    {"spec",        1, 0, 's'},
    {"user-name",   1, 0, 'u'},
    {"verbose",     0, 0, 'v'},
    {"help",        0, 0, 'h'},
    {0, 0, 0, 0}
};

static void
reload_config_handler (int signum)
{
    if (signum == SIGUSR1) {
        mz_config_reload(config);
        syslog(LOG_NOTICE, "reload config file.");
    }
}

int
main (int argc, char *argv[])
{
    int opt;
    int ret;
    char *connection_spec = NULL;
    char *user_name = NULL;
    char *pid_file = NULL;
    char *config_file = NULL;
    bool verbose_mode = false;
    bool daemon_mode = false;
    int option_index;
    struct sigaction reload_config_action;
    struct sigaction old_sighup_action;

    while ((opt = getopt_long(argc, argv, "c:s:u:dhv", long_options, &option_index)) != -1) {
        switch (opt) {
        case 'c':
            config_file = optarg;
            break;
        case 'd':
            daemon_mode = true;
            break;
        case 'h':
            show_usage();
            break;
        case 'p':
            pid_file = optarg;
            break;
        case 's':
            connection_spec = optarg;
            break;
        case 'u':
            user_name = optarg;
            break;
        case 'v':
            verbose_mode = true;
            break;
        default:
            break;
        }
    }

    openlog("milter-zipcrypt", LOG_PID, LOG_MAIL);
    syslog(LOG_NOTICE, "starting milter-zipcrypt.");

    if (config_file) {
        config = mz_config_load(config_file);
        if (!config) {
            syslog(LOG_NOTICE, "config file (%s) is not loaded.", config_file);
        } else {
            if (!connection_spec)
                connection_spec = (char*)mz_config_get_string(config, "connection_spec");
            if (!pid_file)
                pid_file = (char*)mz_config_get_string(config, "pid_file");
            if (!user_name)
                user_name = (char*)mz_config_get_string(config, "user_name");
        }
    }

    if (!connection_spec)
        show_usage();

    if (pid_file) {
        FILE *fd;
        fd = fopen(pid_file, "w");
        if (fd == NULL) {
            syslog(LOG_ERR, "Could not open %s due to %s.",
                   pid_file,
                   strerror(errno));
            goto fail;
        }
        fprintf(fd, "%d\n", getpid());
        fclose(fd);
    }

    if (user_name && !switch_user(user_name))
        goto fail;

    if (daemon_mode && daemon(0, 1) == -1)
        goto fail;

    if (smfi_setconn(connection_spec) == MI_FAILURE)
        goto fail;

    if (smfi_register(smfilter) == MI_FAILURE)
        goto fail;

    if (verbose_mode && smfi_setdbg(6) == MI_FAILURE)
        goto fail;

    reload_config_action.sa_handler = reload_config_handler;
    sigemptyset(&reload_config_action.sa_mask);
    reload_config_action.sa_flags = 0;

    sigaction(SIGUSR1, &reload_config_action, &old_sighup_action);
    ret = smfi_main();
    sigaction(SIGUSR1, &old_sighup_action, NULL);

    syslog(LOG_NOTICE, "exit milter-zipcrypt.");

    return ret;

fail:
    if (config)
        mz_config_free(config);

    if (pid_file)
        unlink(pid_file);

    syslog(LOG_NOTICE, "exit milter-zipcrypt.");

    closelog();
    exit(EXIT_FAILURE);
}

