/* groupserv_common.h - group services common definitions
 * Copyright (C) 2010 Atheme Development Group
 */

#ifndef GROUPSERV_COMMON_H
#define GROUPSERV_COMMON_H

#include "atheme.h"

typedef struct mygroup_ mygroup_t;

#define MG_REGNOLIMIT		0x00000001
#define MG_ACSNOLIMIT		0x00000002
#define MG_OPEN			0x00000004
#define MG_PUBLIC			0x00000008

struct mygroup_ {
	myentity_t ent;

	mowgli_list_t acs;
	time_t regtime;

	unsigned int flags;
};

typedef struct groupacs_ groupacs_t;
unsigned int maxgroups;
unsigned int maxgroupacs;
bool enable_open_groups;
char *join_flags;

#define GA_FOUNDER		0x00000001
#define GA_FLAGS		0x00000002
#define GA_CHANACS		0x00000004
#define GA_MEMOS		0x00000008
#define GA_SET			0x00000010
#define GA_VHOST		0x00000020
#define GA_BAN			0x00000040
#define GA_ALL			(GA_FLAGS | GA_CHANACS | GA_MEMOS | GA_SET | GA_VHOST)

#define PRIV_GROUP_ADMIN "group:admin"
#define PRIV_GROUP_AUSPEX "group:auspex"

struct groupacs_ {
	object_t parent;

	mygroup_t *mg;
	myuser_t *mu;
	unsigned int flags;

	mowgli_node_t gnode;
	mowgli_node_t unode;
};

#endif /* !GROUPSERV_H */