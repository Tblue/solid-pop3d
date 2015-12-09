/* $Id: configfile.h,v 1.5 2000/04/28 16:58:55 jurekb Exp $ */
/*
 *  Solid POP3 - a POP3 server
 *  Copyright (C) 1999  Jerzy Balamut <jurekb@dione.ids.pl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _CONFIGFILE_H
#define _CONFIGFILE_H

#include "includes.h"
#include "const.h"
#include "maildrop.h"

#define OP_BOOLEAN 0
#define OP_STRING 1
#define OP_PERIOD 2
#define OP_EXPIRE 3 /* "never" or OP_PERIOD */

int check_maildrop_type(void *);

#ifdef EXPIRATION
struct expiration {
	time_t expperiod;
	int enabled;
};
extern struct expiration rexp;
extern struct expiration unrexp;
#endif
extern char maildrop_name[];
extern char maildrop_type[];
#ifdef BULLETINS
extern char userbullfile[];
extern char bulldir[];
extern int addbulletins;
#endif
#ifdef APOP
extern char apopservername[];
extern char apopfile[];
#endif
extern unsigned int autologout_time;
#ifdef USERCONFIG
extern int useroverride;
#endif
#ifdef ALLOWROOTLOGIN
extern int allow_root;
#endif
#ifdef APOP
extern int allowapop;
#endif
extern int changegid;
extern unsigned int wccount;
#ifdef MAPPING
extern int domapping;
extern int reqmapping;
extern int authmappeduser;
extern char sp_mapfile[];
extern char sp_usermapprefix[];
extern char mapfileowner[];
#endif
#ifdef NONIPVIRTUALS
extern int allownonip;
#endif
#ifdef CREATEMAILDROP
extern int createmaildrop;
#endif
extern char logpriority[];
#ifdef STATISTICS
extern int logstatistics;
#endif
extern int allowuser;
extern char usermd_delim[2];

struct str_option {
	char *name;
	int op_type;
	void *value;
	size_t valuesize; /* used only with string */
	int (*check_value)(void *);
};
	
extern struct str_option options_set[];

typedef enum {STRING, EOL} tok_type;

struct cf_token {
	char *res_string;
	int res_number;
	int max_size;
	tok_type res_type;
};

#endif /* configfile.h */
