/* $Id: mailbox.h,v 1.2 2000/05/13 13:25:52 jurekb Exp $ */
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

#ifndef _MAILBOX_H
#define _MAILBOX_H

#include "includes.h"
#include "maildrop.h"

void mb_init(void);
void mb_update(void);
void mb_release(void);
void mb_retrieve(unsigned int);
void mb_top(unsigned int, unsigned int);
char *mb_md5_uidl_message(unsigned int, char *);
#ifdef BULLETINS
int mb_add_message(int);
void mb_end_of_adding(void);
#endif

struct mb_message {
	off_t from_where;
	off_t where;
	size_t from_size;
	char digest[16];
};

#endif				/* mailbox.h */
