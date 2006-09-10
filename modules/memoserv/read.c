/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv READ function
 *
 * $Id: read.c 6337 2006-09-10 15:54:41Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/read", FALSE, _modinit, _moddeinit,
	"$Id: read.c 6337 2006-09-10 15:54:41Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_read(sourceinfo_t *si, int parc, char *parv[]);

command_t ms_read = { "READ", "Reads a memo.",
                        AC_NONE, 2, ms_cmd_read };

list_t *ms_cmdtree;
list_t *ms_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ms_cmdtree, "memoserv/main", "ms_cmdtree");
	MODULE_USE_SYMBOL(ms_helptree, "memoserv/main", "ms_helptree");

        command_add(&ms_read, ms_cmdtree);
	help_addentry(ms_helptree, "READ", "help/memoserv/read", NULL);
}

void _moddeinit()
{
	command_delete(&ms_read, ms_cmdtree);
	help_delentry(ms_helptree, "READ");
}

static void ms_cmd_read(sourceinfo_t *si, int parc, char *parv[])
{
	/* Misc structs etc */
	user_t *u = si->su;
	myuser_t *mu = u->myuser, *tmu;
	mymemo_t *memo, *receipt;
	node_t *n;
	uint32_t i = 1, memonum = 0;
	char strfbuf[32];
	struct tm tm;
	
	/* Grab arg */
	char *arg1 = parv[0];
	
	/* Bad/missing arg -- how do I make sure it's a digit they fed me? */
	if (!arg1)
	{
		notice(memosvs.nick, si->su->nick, 
			STR_INSUFFICIENT_PARAMS, "READ");
		
		notice(memosvs.nick, si->su->nick, "Syntax: READ <memo number>");
		return;
	}
	else
		memonum = atoi(arg1);
	
	/* user logged in? */
	if (mu == NULL)
	{
		notice(memosvs.nick, si->su->nick, "You are not logged in.");
		return;
	}
	
	/* Check to see if any memos */
	if (!mu->memos.count)
	{
		notice(memosvs.nick, si->su->nick, "You have no memos.");
		return;
	}
	
	/* Is arg1 an int? */
	if (!memonum)
	{
		notice(memosvs.nick, si->su->nick, "Invalid message index.");
		return;
	}
	
	/* Check to see if memonum is greater than memocount */
	if (memonum > mu->memos.count)
	{
		notice(memosvs.nick, si->su->nick, "Invalid message index.");
		return;
	}

	/* Go to reading memos */	
	LIST_FOREACH(n, mu->memos.head)
	{
		if (i == memonum)
		{
			memo = (mymemo_t*) n->data;
			tm = *localtime(&memo->sent);
			strftime(strfbuf, sizeof(strfbuf) - 1, 
				"%b %d %H:%M:%S %Y", &tm);
			
			if (memo->status == MEMO_NEW)
			{
				memo->status = MEMO_READ;
				mu->memoct_new--;
				tmu = myuser_find(memo->sender);
				
				/* If the sender is logged in, tell them the memo's been read */
				if (strcasecmp(memosvs.nick,memo->sender) && (tmu != NULL) && (tmu->logins.count > 0))
					myuser_notice(memosvs.nick, tmu, "%s has read your memo, which was sent at %s", si->su->nick, strfbuf);			
				else
				{	
					/* If they have an account, their inbox is not full and they aren't memoserv */
					if ( (tmu != NULL) && (tmu->memos.count < me.mdlimit) && strcasecmp(memosvs.nick,memo->sender))
					{
						/* Malloc and populate memo struct */
						receipt = smalloc(sizeof(mymemo_t));
						receipt->sent = CURRTIME;
						receipt->status = MEMO_NEW;
						strlcpy(receipt->sender,memosvs.nick,NICKLEN);
						snprintf(receipt->text, MEMOLEN, "%s has read a memo from you sent at %s", si->su->nick, strfbuf);
						
						/* Attach to their linked list */
						n = node_create();
						node_add(receipt, n, &tmu->memos);
						tmu->memoct_new++;
					}
				}
			}
		
			notice(memosvs.nick, si->su->nick, 
				"\2Memo %d - Sent by %s, %s\2",i,memo->sender, strfbuf);
			
			notice(memosvs.nick, si->su->nick, 
				"------------------------------------------");
			
			notice(memosvs.nick, si->su->nick, "%s", memo->text);
			
			return;
		}
		
		i++;
	}
}
