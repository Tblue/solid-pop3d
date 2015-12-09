/* $Id: maildir.h,v 1.3 2000/05/13 13:25:52 jurekb Exp $ */
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

#ifndef _MAILDIR_H
#define _MAILDIR_H

#include "maildrop.h"

void mdir_init(void);
void mdir_update(void);
void mdir_release(void);
void mdir_retrieve(unsigned int);
void mdir_top(unsigned int, unsigned int);
char *mdir_md5_uidl_message(unsigned int, char *);
#ifdef BULLETINS
int mdir_add_message(int fd);
void mdir_end_of_adding(void);
#endif

struct mdir_message {
	char *filename;
};

#endif				/* maildir.h */
