/* $Id: cmds.h,v 1.1.1.1 2000/04/12 20:52:26 jurekb Exp $ */
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

#ifndef _CMDS_H
#define _CMDS_H

#include "response.h"

typedef void (*cmd_handler) (char *);

struct s_cmd {
	char *name;
	cmd_handler handler;
};

const struct s_cmd *cmd_lookup(const struct s_cmd *cmdtbl, char *cmd);
void ignore_cmd(char *arg);
void not_implemented(char *arg);

#endif				/*
				   * cmds.h 
				 */
