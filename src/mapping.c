static const char rcsid[] = "$Id: mapping.c,v 1.2 2000/04/17 19:38:04 jurekb Exp $";
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
#include "mapping.h"
#include "fdfgets.h"
#include "log.h"
#include <fcntl.h>
#include <pwd.h>
#ifdef HAVE_GRP_H
#include <grp.h>
#endif

/* result should be a (MAXARGLN+1)-byte buffer */
int map_finduser(char *mapfile, char *username, char *result) {
	int fd, linenr = 1, tmp2, tmp3;
	ssize_t lcount;
	char linebuf[MAXARGLN + 1 + MAXARGLN + 1];
		/* username:maptouser\n */
	char *tmp;
	
	if ((fd = open(mapfile, O_RDONLY)) < 0) {
		pop_log(pop_priority, "map: can't open file: %.1024s", mapfile);
		pop_error("map: open");
		return -1;
	};
	fd_initfgets();
	while ((lcount = fd_fgets(linebuf, sizeof(linebuf), fd)) > 0) {
		if ((linebuf[lcount - 1] != '\n') && (lcount == sizeof(linebuf))) { /* line too long */
			pop_log(pop_priority, "map: line %d in %.1024s is too long", linenr, mapfile);
			lcount = -1;
			break;
		};
		if (linebuf[lcount - 1] == '\n')
			linebuf[lcount - 1] = 0;
		else
			linebuf[lcount] = 0; /* lcount < sizeof(linebuf) */
		tmp3 = 0;
		for (tmp2 = 0; tmp2 < lcount; tmp2++)
			if (linebuf[tmp2] == ':')
				tmp3++;
		if (tmp3 != 1) {
			pop_log(pop_priority, "map: syntax error in line nr %d in file %.1024s", linenr, mapfile);
			lcount = -1;
			break;
		};
		*(tmp = strchr(linebuf, ':')) = 0;
		if (strlen(linebuf) > MAXARGLN) {
			pop_log(pop_priority, "map: user name too long in line %d in %.1024s", linenr, mapfile);
			lcount = -1;
			break;
		};
		if (strcmp(linebuf, username) == 0) { /* parse line */
			if (strlen(++tmp) > MAXARGLN) {
				pop_log(pop_priority, "map: wrong mapped user name entry in line %d in %.1024s", linenr, mapfile);
				lcount = -1;
				break;
			};
			strcpy(result, tmp);
			break;
		};
		linenr++;
	};
	close(fd);
	memset(linebuf, 0, sizeof(linebuf));
	return (lcount <= 0) ? -1 : 0;
};
