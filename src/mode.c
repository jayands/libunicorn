#include <string.h>
#include <mowgli.h>

#include "unicorn.h"


irc_prefix_t *irc_prefix_create(irc_isupport_t *isupport)
{
	irc_prefix_t *pfx;

	pfx = mowgli_alloc(sizeof(*pfx));
	if (pfx == NULL)
		return NULL;

	pfx->bv = 0;
	pfx->isupport = isupport;

	return pfx;
}

int irc_prefix_destroy(irc_prefix_t *pfx)
{
	if (pfx == NULL)
		return -1;

	mowgli_free(pfx);
	return 0;
}


char *irc_prefix_isupport_mode(irc_prefix_t *pfx)
{
	if (pfx->isupport == NULL || pfx->isupport->prefix.mode == NULL
			|| pfx->isupport->prefix.mode->str == NULL)
		return "ov";

	return pfx->isupport->prefix.mode->str;
}

char *irc_prefix_isupport_prefix(irc_prefix_t *pfx)
{
	if (pfx->isupport == NULL || pfx->isupport->prefix.prefix == NULL
			|| pfx->isupport->prefix.prefix->str == NULL)
		return "@+";

	return pfx->isupport->prefix.prefix->str;
}


int irc_prefix_set(irc_prefix_t *pfx, char mode)
{
	char *p, *q;

	if (pfx == NULL)
		return -1;

	p = irc_prefix_isupport_mode(pfx);
	q = strchr(p, mode);

	if (q == NULL)
		return -1;

	pfx->bv |= (1 << (q - p));

	return 0;
}

int irc_prefix_clear(irc_prefix_t *pfx, char mode)
{
	char *p, *q;

	if (pfx == NULL)
		return -1;

	p = irc_prefix_isupport_mode(pfx);
	q = strchr(p, mode);

	if (q == NULL)
		return -1;

	pfx->bv &= ~(1 << (q - p));

	return 0;
}


char irc_prefix_char(irc_prefix_t *pfx)
{
	char *p;
	int i, n;

	if (pfx == NULL)
		return ' ';

	p = irc_prefix_isupport_prefix(pfx);

	n = strlen(p);
	if (n > (sizeof(pfx->bv) << 3))
		n = (sizeof(pfx->bv) << 3);

	for (i=0; i<n; i++) {
		if ((pfx->bv & (1 << i)) != 0) {
			return p[i];
		}
	}

	return ' ';
}
