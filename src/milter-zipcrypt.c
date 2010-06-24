/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <libmilter/mfapi.h>

#include "mz-base64.h"
#include "mz-utils.h"
#include "mz-zip.h"

struct MzPriv {
    char *boundary;
    unsigned char *body;
    size_t body_length;
};

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

    if (strcasecmp(name, "Content-type"))
        return SMFIS_CONTINUE;

    if (!strstr(value, "multipart/mixed;"))
        return SMFIS_CONTINUE;

    boundary = get_boundary(value);
    if (!boundary)
        return SMFIS_ACCEPT;

    priv = malloc(sizeof(*priv));
    if (!priv) {
        free(boundary);
        return SMFIS_ACCEPT;
    }

    memset(priv, 0, sizeof(*priv));
    priv->boundary = boundary;
    priv->body = NULL;
    priv->body_length = 0;

    smfi_setpriv(context, priv);

    return SMFIS_CONTINUE;
}

static sfsistat
append_body (struct MzPriv *priv, unsigned char *chunk, size_t size)
{
    if (!priv->body) {
        priv->body = malloc(size);
        if (!priv->body)
            return SMFIS_SKIP;
        memcpy(priv->body, chunk, size);
        priv->body_length = size;
    } else {
        priv->body = realloc(priv->body, priv->body_length + size);
        if (!priv->body)
            return SMFIS_SKIP;
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
_send_headers (SMFICTX *context)
{
    char *content_type;
    char *content_disposition;

    content_type = "Content-Type: application/zip; name=\"attachment.zip\"\r\n";
    content_disposition = "Content-Disposition: attachment; filename=\"attachment.zip\"\r\n";

    smfi_replacebody(context,
                     (unsigned char*)content_type,
                     strlen(content_type));
    smfi_replacebody(context,
                     (unsigned char*)content_disposition,
                     strlen(content_disposition));
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
    int state = 0;
    int save = 0;
    MzZipStreamStatus status;

    _send_headers(context);

    zip = mz_zip_stream_create("password");
    mz_zip_stream_begin_archive(zip);
    for (node = attachments; node; node = mz_list_next(node)) {
        MzAttachment *attachment = node->data;
        unsigned int zip_data_position = 0;
        unsigned int processed_size;

        mz_zip_stream_begin_file(zip,
                                 attachment->filename,
                                 zip_output,
                                 ZIP_BUFFER_SIZE,
                                 &written_size);
        _replace_body_with_base64(context, zip_output, written_size, &state, &save);

        do {
            status = mz_zip_stream_process_file_data(zip,
                                                     attachment->data + zip_data_position,
                                                     attachment->data_length - zip_data_position,
                                                     zip_output,
                                                     ZIP_BUFFER_SIZE,
                                                     &processed_size,
                                                     &written_size);
            if (status != MZ_ZIP_STREAM_STATUS_SUCCESS &&
                status != MZ_ZIP_STREAM_STATUS_REMAIN_OUTPUT_DATA) {
                mz_zip_stream_destroy(zip);
                return SMFIS_TEMPFAIL;
            }
            zip_data_position += processed_size;
            _replace_body_with_base64(context, zip_output, written_size, &state, &save);
        } while (zip_data_position < attachment->data_length);

        mz_zip_stream_end_file(zip,
                               zip_output,
                               ZIP_BUFFER_SIZE,
                               &written_size);
        _replace_body_with_base64(context, zip_output, written_size, &state, &save);
    }

    mz_zip_stream_end_archive(zip,
                              zip_output,
                              ZIP_BUFFER_SIZE,
                              &written_size);
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
}

static sfsistat
_send_body (SMFICTX *context, struct MzPriv *priv, MzList *attachments)
{
    MzAttachment *body = attachments->data;
    return smfi_replacebody(context,
                            priv->body,
                            ((unsigned char*)body->boundary_start_position - priv->body));
}

static sfsistat
_eom (SMFICTX *context)
{
    struct MzPriv *priv;
    MzList *attachments;

    priv = (struct MzPriv*)smfi_getpriv(context);
    if (!priv)
        return SMFIS_ACCEPT;

    if (!priv->boundary)
        return SMFIS_ACCEPT;

    if (!priv->body)
        return SMFIS_ACCEPT;

    smfi_addheader(context, "X-ZIP-Crypted", "Yes");

    attachments = mz_utils_extract_attachments((char*)priv->body, priv->boundary);
    if (!attachments)
        return SMFIS_CONTINUE;

    _send_body(context, priv, attachments);
    _replace_with_crypted_data(context, priv, attachments);
    mz_list_free_with_free_func(attachments, (MzListElementFreeFunc)mz_attachment_free);

    return SMFIS_CONTINUE;
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

    if (priv->boundary) {
        free(priv->boundary);
        priv->boundary = NULL;
    }

    if (priv->body) {
        free(priv->body);
        priv->body = NULL;
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

int
main (int argc, char *argv[])
{
    int opt;
    int ret;
    char *connection_spec = NULL;
    bool verbose_mode = false;

    while ((opt = getopt(argc, argv, "s:v")) != -1) {
        switch (opt) {
        case 's':
            connection_spec = strdup(optarg);
            break;
        case 'v':
            verbose_mode = true;
            break;
        default:
            break;
        }
    }

    if (smfi_setconn(connection_spec) == MI_FAILURE)
        exit(EXIT_FAILURE);

    if (smfi_register(smfilter) == MI_FAILURE)
        exit(EXIT_FAILURE);

    if (verbose_mode && smfi_setdbg(6) == MI_FAILURE)
        exit(EXIT_FAILURE);

    openlog("milter-zipcrypt", LOG_PID, LOG_MAIL);
    syslog(LOG_NOTICE, "starting milter-zipcrypt.");
    ret = smfi_main();
    syslog(LOG_NOTICE, "exit milter-zipcrypt.");
    closelog();

    return ret;
}

