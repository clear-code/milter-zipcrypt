/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <stdlib.h>
#include <string.h>
#include <libmilter/mfapi.h>
#include <zlib.h>

struct MzPriv {
	char *boundary;
};

sfsistat
mz_envfrom (SMFICTX *context, char **froms)
{
    return SMFIS_CONTINUE;
}

sfsistat
mz_envrcpt (SMFICTX *context, char **recipients)
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

sfsistat
mz_header (SMFICTX *context, char *name, char *value)
{
    struct MzPriv *priv;
    char *boundary;

    if (strcmp(name, "Content-type"))
        return SMFIS_ACCEPT;

    if (!strstr(value, "multipart/mixed;"))
        return SMFIS_ACCEPT;

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

	smfi_setpriv(context, priv);

    return SMFIS_CONTINUE;
}

sfsistat
mz_body (SMFICTX *context, unsigned char *chunk, size_t size)
{
    return SMFIS_CONTINUE;
}

sfsistat
mz_eom (SMFICTX *context)
{
    return SMFIS_CONTINUE;
}

sfsistat
mz_abort (SMFICTX *context)
{
    return mz_cleanup (context);
}

sfsistat
mz_close (SMFICTX *context)
{
    return SMFIS_CONTINUE;
}

sfsistat
mz_cleanup (SMFICTX *context)
{
	struct MzPriv *priv;
    
    priv = (struct MzPriv*)smfi_getpriv(context);
    if (!priv)
        return SMFIS_CONTINUE;

    if (priv->boundary) {
        free(priv->boundary);
        priv->boundary = NULL;
    }
    free(priv);
    priv = NULL;
    smfi_setpriv(context, NULL);

    return SMFIS_CONTINUE;
}

