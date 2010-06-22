/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libmilter/mfapi.h>

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

static sfsistat
_eom (SMFICTX *context)
{
    struct MzPriv *priv;

    priv = (struct MzPriv*)smfi_getpriv(context);
    if (!priv)
        return SMFIS_ACCEPT;

    if (!priv->boundary)
        return SMFIS_ACCEPT;

    if (!priv->body)
        return SMFIS_ACCEPT;

    smfi_addheader(context, "X-ZIP-Crypted", "Yes");
    smfi_replacebody(context, priv->body, priv->body_length);

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
    NULL  /* negotiate */
};

int
main (int argc, char *argv[])
{
    int opt;
    char *connection_spec = NULL;

    while ((opt = getopt(argc, argv, "s:")) != -1) {
        switch (opt) {
        case 's':
            connection_spec = strdup(optarg);
            break;
        default:
            break;
        }
    }

    if (smfi_setconn(connection_spec) == MI_FAILURE) {
        exit(EXIT_FAILURE);
    }

    if (smfi_register(smfilter) == MI_FAILURE) {
        exit(EXIT_FAILURE);
    }

    return smfi_main();
}

