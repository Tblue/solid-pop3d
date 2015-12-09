static const char rcsid[] = "$Id: cmds.c,v 1.2 2000/04/28 16:58:55 jurekb Exp $";
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

#include "includes.h"
#include "cmds.h"
#include "response.h"

extern void check_wccount(void);

char upcase(char what)
{				/*
				 * toupper() uses locale so is slower
				 */
	if ((what <= 'z') && (what >= 'a'))
		return what - ('a' - 'A');
	else
		return what;
}

const struct s_cmd *
 cmd_lookup(const struct s_cmd *cmdtbl, char *cmd)
{
	int actual = 0, length, tmp;

	while (cmdtbl[actual].name != NULL) {
		length = strlen(cmdtbl[actual].name);
		if (strlen(cmd) < length) {
			actual++;
			continue;
		};
		tmp = 0;
		while (cmdtbl[actual].name[tmp] == upcase(cmd[tmp]))
			tmp++;
		if ((tmp > length) ||
		    ((tmp == length) && (cmd[length] == ' ')))
			return &cmdtbl[actual];
		actual++;
	};
	return NULL;
}

void ignore_cmd(char *arg)
{
	send_ok("command ignored");
}

void not_implemented(char *arg) {
	send_error("command not implemented yet");
	check_wccount();
}
