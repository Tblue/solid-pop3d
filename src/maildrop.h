/* $Id: maildrop.h,v 1.3 2000/05/13 13:25:52 jurekb Exp $ */
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

#ifndef _MAILDROP_H
#define _MAILDROP_H

#include "includes.h"
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif


typedef void (*_md_cleanup) (void);

typedef void (*_md_init) (void);
typedef void (*_md_update) (void);	/* Remove marked messages */
typedef void (*_md_release) (void);	/* Release any locks */
typedef void (*_md_retrieve) (unsigned int);
typedef void (*_md_top) (unsigned int, unsigned int);	/* optional */
typedef char *(*_md_md5_uidl_message) (unsigned int, char *); /* optional */
#ifdef BULLETINS
typedef int (*_md_add_message)(int fd);
typedef void (*_md_end_of_adding)(void);
#endif

struct str_maildrop {
	_md_init md_init;
	_md_update md_update;
	_md_release md_release;
	_md_retrieve md_retrieve;
	_md_top md_top;
	_md_md5_uidl_message md_md5_uidl_message;
#ifdef BULLETINS
	_md_add_message md_add_message;
	_md_end_of_adding md_end_of_adding;
#endif
};

struct message {
	size_t crlfsize;
	size_t size;
	int deleted;
	void *md_specific;
	time_t msg_time;
	int cread;
};

struct str_maildrop_name {
	char *name;
	struct str_maildrop *mdrop;
};

int md_alloc(int size);
int md_realloc(int size);
struct str_maildrop *find_maildrop(char *name);
void md_free(void);

void md_stat(void);
void md_reset(void);
void md_delete(unsigned int nr);
void md_uidl(unsigned int nr, _md_md5_uidl_message md_md5_uidl_message);
void md_list(unsigned int nr);
void md_top(unsigned int nr, unsigned int lcount, int fd, _md_cleanup cleanup);
void md_retrieve(unsigned int nr, int fd, _md_cleanup cleanup);

void md_end_reply(_md_cleanup cleanup);
#endif				/* maildrop.h */
