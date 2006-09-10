/*
 * Copyright (c) 2006 Robin Burchell <surreal.w00t@gmail.com>
 * Rights to this code are as defined in doc/LICENSE.
 *
 * Regexp-based AKILL implementation.
 *
 * $Id: rakill.c 6337 2006-09-10 15:54:41Z pippijn $
 */

/*
 * @makill regex!here.+ [akill reason]
 *  Matches `nick!user@host realname here' for each client against a given regex, and places akills.
 *  CHECK REGEX USING @RMATCH FIRST!
 */
#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/rakill", FALSE, _modinit, _moddeinit,
	"$Id: rakill.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *os_cmdtree;
list_t *os_helptree;

static void os_cmd_rakill(sourceinfo_t *si, int parc, char *parv[]);

command_t os_rakill = { "RAKILL", "Sets a group of AKILLs against users matching a specific regex pattern.", PRIV_MASS_AKILL, 1, os_cmd_rakill };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_rakill, os_cmdtree);
	help_addentry(os_helptree, "RAKILL", "help/oservice/rakill", NULL);
}

void _moddeinit(void)
{
	command_delete(&os_rakill, os_cmdtree);
	help_delentry(os_helptree, "RAKILL");
}

static void os_cmd_rakill(sourceinfo_t *si, int parc, char *parv[])
{
	regex_t *regex;
	char usermask[512];
	uint32_t matches = 0;
	uint32_t i = 0;
	node_t *n;
	user_t *u;
	char *args = parv[0];
	char *pattern;
	char *reason;
	user_t *source = si->su;
	int flags = 0;

	if (source == NULL)
		return;

	/* make sure they could have done RMATCH */
	if (!has_priv(source, PRIV_USER_AUSPEX))
	{
		notice(opersvs.nick, si->su->nick, "You do not have %s privilege.", PRIV_USER_AUSPEX);
		return;
	}

	if (args == NULL)
	{
		notice(opersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "RAKILL");
		notice(opersvs.nick, si->su->nick, "Syntax: RAKILL /<regex>/[i] <reason>");
		return;
	}

	pattern = regex_extract(args, &args, &flags);
	if (pattern == NULL)
	{
		notice(opersvs.nick, si->su->nick, STR_INVALID_PARAMS, "RAKILL");
		notice(opersvs.nick, si->su->nick, "Syntax: RAKILL /<regex>/[i] <reason>");
		return;
	}

	reason = args;
	while (*reason == ' ')
		reason++;
	if (*reason == '\0')
	{
		notice(opersvs.nick, si->su->nick, STR_INSUFFICIENT_PARAMS, "RAKILL");
		notice(opersvs.nick, si->su->nick, "Syntax: RAKILL /<regex>/[i] <reason>");
		return;
	}

	regex = regex_create(pattern, flags);
	if (regex == NULL)
	{
		notice(opersvs.nick, si->su->nick, "The provided regex \2%s\2 is invalid.", pattern);
		return;
	}

	sprintf(usermask, "%s!%s@%s %s", source->nick, source->user, source->host, source->gecos);
	if (regex_match(regex, (char *)usermask))
	{
		regex_destroy(regex);
		notice(opersvs.nick, si->su->nick, "The provided regex matches you, refusing RAKILL.");
		snoop("RAKILL:REFUSED: \2%s\2 by \2%s\2 (%s) (matches self)", pattern, si->su->nick, reason);
		wallops("\2%s\2 attempted to do RAKILL on \2%s\2 matching self",
				si->su->nick, pattern);
		logcommand(opersvs.me, si->su, CMDLOG_ADMIN, "RAKILL %s %s (refused, matches self)", pattern, reason);
		return;
	}

	snoop("RAKILL: \2%s\2 by \2%s\2 (%s)", pattern, si->su->nick, reason);

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, userlist[i].head)
		{
			u = (user_t *) n->data;

			sprintf(usermask, "%s!%s@%s %s", u->nick, u->user, u->host, u->gecos);

			if (regex_match(regex, (char *)usermask))
			{
				/* match */
				notice(opersvs.nick, si->su->nick, "\2Match:\2  %s!%s@%s %s - akilling", u->nick, u->user, u->host, u->gecos);
				kline_sts("*", "*", u->host, 604800, reason);
				matches++;
			}
		}
	}
	
	regex_destroy(regex);
	notice(opersvs.nick, si->su->nick, "\2%d\2 matches for %s akilled.", matches, pattern);
	logcommand(opersvs.me, si->su, CMDLOG_ADMIN, "RAKILL %s %s (%d matches)", pattern, reason, matches);
}
