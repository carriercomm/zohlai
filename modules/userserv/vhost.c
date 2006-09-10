/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * VHost management! (ratbox only right now.)
 *
 * $Id: vhost.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/vhost", FALSE, _modinit, _moddeinit,
	"$Id: vhost.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *us_cmdtree, *us_helptree;

static void vhost_on_identify(void *vptr);
static void us_cmd_vhost(sourceinfo_t *si, int parc, char *parv[]);

command_t us_vhost = { "VHOST", "Manages user virtualhosts.", PRIV_USER_VHOST, 2, us_cmd_vhost };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(us_cmdtree, "userserv/main", "us_cmdtree");
	MODULE_USE_SYMBOL(us_helptree, "userserv/main", "us_helptree");

	hook_add_event("user_identify");
	hook_add_hook("user_identify", vhost_on_identify);
	command_add(&us_vhost, us_cmdtree);
	help_addentry(us_helptree, "VHOST", "help/userserv/vhost", NULL);
}

void _moddeinit(void)
{
	hook_del_hook("user_identify", vhost_on_identify);
	command_delete(&us_vhost, us_cmdtree);
	help_delentry(us_helptree, "VHOST");
}

static void do_sethost_all(myuser_t *mu, char *host)
{
	node_t *n;
	user_t *u;
	char luhost[BUFSIZE];

	LIST_FOREACH(n, mu->logins.head)
	{
		u = n->data;

		if (!strcmp(u->vhost, host))
			continue;
		strlcpy(u->vhost, host, HOSTLEN);
		notice(usersvs.nick, u->nick, "Setting your host to \2%s\2.",
			u->vhost);
		sethost_sts(usersvs.nick, u->nick, u->vhost);
		strlcpy(luhost, u->user, BUFSIZE);
		strlcat(luhost, "@", BUFSIZE);
		strlcat(luhost, u->vhost, BUFSIZE);
		metadata_add(mu, METADATA_USER, "private:host:vhost", luhost);
	}
}

static void do_sethost(user_t *u, char *host)
{
	char luhost[BUFSIZE];

	if (!strcmp(u->vhost, host))
		return;
	strlcpy(u->vhost, host, HOSTLEN);
	notice(usersvs.nick, u->nick, "Setting your host to \2%s\2.",
		u->vhost);
	sethost_sts(usersvs.nick, u->nick, u->vhost);
	strlcpy(luhost, u->user, BUFSIZE);
	strlcat(luhost, "@", BUFSIZE);
	strlcat(luhost, u->vhost, BUFSIZE);
	metadata_add(u->myuser, METADATA_USER, "private:host:vhost", luhost);
}

static void do_restorehost_all(myuser_t *mu)
{
	node_t *n;
	user_t *u;
	char luhost[BUFSIZE];

	LIST_FOREACH(n, mu->logins.head)
	{
		u = n->data;

		if (!strcmp(u->vhost, u->host))
			continue;
		strlcpy(u->vhost, u->host, HOSTLEN);
		notice(usersvs.nick, u->nick, "Setting your host to \2%s\2.",
			u->host);
		sethost_sts(usersvs.nick, u->nick, u->host);
		strlcpy(luhost, u->user, BUFSIZE);
		strlcat(luhost, "@", BUFSIZE);
		strlcat(luhost, u->host, BUFSIZE);
		metadata_add(mu, METADATA_USER, "private:host:vhost", luhost);
	}
}

/* VHOST <nick> [host] */
static void us_cmd_vhost(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *host = parv[1];
	myuser_t *mu;

	if (!target)
	{
		notice(usersvs.nick, si->su->nick, STR_INVALID_PARAMS, "VHOST");
		notice(usersvs.nick, si->su->nick, "Syntax: VHOST <nick> [vhost]");
		return;
	}

	/* find the user... */
	if (!(mu = myuser_find_ext(target)))
	{
		notice(usersvs.nick, si->su->nick, "\2%s\2 is not a registered account.", target);
		return;
	}

	/* deletion action */
	if (!host)
	{
		metadata_delete(mu, METADATA_USER, "private:usercloak");
		notice(usersvs.nick, si->su->nick, "Deleted vhost for \2%s\2.", target);
		snoop("VHOST:REMOVE: \2%s\2 by \2%s\2", target, si->su->nick);
		logcommand(usersvs.me, si->su, CMDLOG_ADMIN, "VHOST REMOVE %s", target);
		do_restorehost_all(mu);
		return;
	}

	/* Never ever allow @!?* as they have special meaning in all ircds */
	if (strchr(host, '@') || strchr(host, '!') || strchr(host, '?') ||
			strchr(host, '*'))
	{
		notice(usersvs.nick, si->su->nick, "The vhost provided contains invalid characters.");
		return;
	}
	if (strlen(host) >= HOSTLEN)
	{
		notice(usersvs.nick, si->su->nick, "The vhost provided is too long.");
		return;
	}
	/* XXX more checks here, perhaps as a configurable regexp? */

	metadata_add(mu, METADATA_USER, "private:usercloak", host);
	notice(usersvs.nick, si->su->nick, "Assigned vhost \2%s\2 to \2%s\2.",
			host, target);
	snoop("VHOST:ASSIGN: \2%s\2 to \2%s\2 by \2%s\2", host, target, si->su->nick);
	logcommand(usersvs.me, si->su, CMDLOG_ADMIN, "VHOST ASSIGN %s %s",
			target, host);
	do_sethost_all(mu, host);
	return;
}

static void vhost_on_identify(void *vptr)
{
	user_t *u = vptr;
	myuser_t *mu = u->myuser;
	metadata_t *md;

	/* NO CLOAK?!*$*%*&&$(!& */
	if (!(md = metadata_find(mu, METADATA_USER, "private:usercloak")))
		return;

	do_sethost(u, md->value);
}
