/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv FORWARD function
 *
 * $Id: forward.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/forward", FALSE, _modinit, _moddeinit,
	"$Id: forward.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_forward(sourceinfo_t *si, int parc, char *parv[]);

command_t ms_forward = { "FORWARD", "Forwards a memo.",
                        AC_NONE, 2, ms_cmd_forward };

list_t *ms_cmdtree;
list_t *ms_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ms_cmdtree, "memoserv/main", "ms_cmdtree");
	MODULE_USE_SYMBOL(ms_helptree, "memoserv/main", "ms_helptree");

        command_add(&ms_forward, ms_cmdtree);
	help_addentry(ms_helptree, "forward", "help/memoserv/forward", NULL);
}

void _moddeinit()
{
	command_delete(&ms_forward, ms_cmdtree);
	help_delentry(ms_helptree, "FORWARD");
}

static void ms_cmd_forward(sourceinfo_t *si, int parc, char *parv[])
{
	/* Misc structs etc */
	user_t *u = si->su, *tu;
	myuser_t *mu = u->myuser, *tmu;
	mymemo_t *memo, *newmemo;
	node_t *n, *temp;
	uint8_t i = 1, memonum = 0;
	
	/* Grab args */
	char *target = parv[0];
	char *arg = parv[1];
	
	/* Arg validator */
	if (!target || !arg)
	{
		notice(memosvs.nick, si->su->nick, 
			STR_INSUFFICIENT_PARAMS, "FORWARD");
		
		notice(memosvs.nick, si->su->nick, 
			"Syntax: FORWARD <account> <memo number>");
		
		return;
	}
	else
		memonum = atoi(arg);
	
	/* user logged in? */
	if (mu == NULL)
	{
		notice(memosvs.nick, si->su->nick, "You are not logged in.");
		return;
	}
	
	if (mu->flags & MU_WAITAUTH)
	{
		notice(memosvs.nick, si->su->nick, "You need to verify your email address before you may send memos.");
		return;
	}

	/* Check to see if any memos */
	if (!mu->memos.count)
	{
		notice(memosvs.nick, si->su->nick, "You have no memos to forward.");
		return;
	}

	/* Check to see if target user exists */
	if (!(tmu = myuser_find_ext(target)))
	{
		notice(memosvs.nick, si->su->nick, "%s is not registered.", target);
		return;
	}
	
	/* Make sure target isn't sender */
	if (mu == tmu)
	{
		notice(memosvs.nick, si->su->nick, "You cannot send yourself a memo.");
		return;
	}
	
	/* Make sure arg is an int */
	if (!memonum)
	{
		notice(memosvs.nick, si->su->nick, "Invalid message index.");
		return;
	}
	
	/* check if targetuser has nomemo set */
	if (tmu->flags & MU_NOMEMO)
	{
		notice(memosvs.nick, si->su->nick,
			"\2%s\2 does not wish to receive memos.", target);

		return;
	}

	/* Check to see if memo n exists */
	if (memonum > mu->memos.count)
	{
		notice(memosvs.nick, si->su->nick, "Invalid memo number.");
		return;
	}
	
	/* Check to make sure target inbox not full */
	if (tmu->memos.count >= me.mdlimit)
	{
		notice(memosvs.nick, si->su->nick, "Target inbox is full.");
		logcommand(memosvs.me, u, CMDLOG_SET, "failed FORWARD to %s (target inbox full)", tmu->name);
		return;
	}

	/* rate limit it -- jilles */
	if (CURRTIME - mu->memo_ratelimit_time > MEMO_MAX_TIME)
		mu->memo_ratelimit_num = 0;
	if (mu->memo_ratelimit_num > MEMO_MAX_NUM)
	{
		notice(memosvs.nick, si->su->nick, "Too many memos; please wait a while and try again");
		return;
	}
	mu->memo_ratelimit_num++;
	mu->memo_ratelimit_time = CURRTIME;

	/* Make sure we're not on ignore */
	LIST_FOREACH(n, tmu->memo_ignores.head)
	{
		if (!strcasecmp((char *)n->data, mu->name))
		{
			/* Lie... change this if you want it to fail silent */
			logcommand(memosvs.me, u, CMDLOG_SET, "failed FORWARD to %s (on ignore list)", tmu->name);
			notice(memosvs.nick, si->su->nick, "The memo has been successfully forwarded to %s.", target);
			return;
		}
	}
	logcommand(memosvs.me, u, CMDLOG_SET, "FORWARD to %s", tmu->name);
	
	/* Go to forwarding memos */
	LIST_FOREACH(n, mu->memos.head)
	{
		if (i == memonum)
		{
			/* should have some function for send here...  ask nenolod*/
			memo = (mymemo_t *)n->data;
			newmemo = smalloc(sizeof(mymemo_t));
			
			/* Create memo */
			newmemo->sent = CURRTIME;
			newmemo->status = MEMO_NEW;
			strlcpy(newmemo->sender,mu->name,NICKLEN);
			strlcpy(newmemo->text,memo->text,MEMOLEN);
			
			/* Create node, add to their linked list of memos */
			temp = node_create();
			node_add(newmemo, temp, &tmu->memos);
			tmu->memoct_new++;
		
			/* Should we email this? */
			if (tmu->flags & MU_EMAILMEMOS)
			{
				if (sendemail(u, EMAIL_MEMO, tmu, memo->text))
				{
					notice(memosvs.nick, si->su->nick, "Your memo has been emailed to %s.", target);
					return;
				}
			}
		}
		i++;
	}

	/* Note: do not disclose other nicks they're logged in with
	 * -- jilles */
	tu = user_find_named(target);
	if (tu != NULL && tu->myuser == tmu)
	{
		notice(memosvs.nick, si->su->nick, "%s is currently online, and you may talk directly, by sending a private message.", target);
	}
	if (!irccmp(si->su->nick, mu->name))
		myuser_notice(memosvs.nick, tmu, "You have a new forwarded memo from %s.", mu->name);
	else
		myuser_notice(memosvs.nick, tmu, "You have a new forwarded memo from %s (nick: %s).", mu->name, si->su->nick);

	notice(memosvs.nick, si->su->nick, "The memo has been successfully forwarded to %s.", target);
	return;
}
