static const char rcsid[] = "$Id: fdfgets.c,v 1.2 2000/04/28 16:58:55 jurekb Exp $";
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

#include "fdfgets.h"

char fgets_buf[10240], *fgets_where;
size_t fgets_count, fgets_empty;

void fd_initfgets(void)
{
	fgets_count = fgets_empty = 0;
	fgets_where = fgets_buf;
}

ssize_t fd_fgets(char *s, size_t size, int fd)
{
	size_t tmp, tmp2;
	ssize_t cread;
	char *newline;

	tmp = 0;
	tmp2 = ((size < fgets_count) ? size : fgets_count);
	newline = NULL;
	if (size > sizeof(fgets_buf))
		return -1; /* FIXME */
	if ((tmp2 == 0) || ((newline = memchr(fgets_where, '\n', tmp2)) == NULL)) {
		tmp = tmp2;
		if (tmp > 0) {
			memcpy(s, fgets_where, tmp);
			fgets_count -= tmp;
			fgets_empty += tmp;
		};
		if (fgets_count == 0) {
			fgets_where = fgets_buf;
			fgets_empty = 0;
		} else
			fgets_where = fgets_buf + fgets_empty;
		if (tmp == size)
			return size;
		size -= tmp;
		fgets_empty = 0;	
		fgets_where = fgets_buf;
		if ((cread = read(fd, fgets_buf, sizeof(fgets_buf))) < 0)
			return -1;
		fgets_count = cread;
		if (fgets_count == 0)
			return tmp;
	};
	if (size > fgets_count)
		size = fgets_count;
	if (newline == NULL)
		newline = memchr(fgets_where, '\n', size);
	if (newline != NULL)
		size = newline - fgets_where + 1;
	memcpy(s + tmp, fgets_where, size);
	fgets_count -= size;
	fgets_empty += size;
	fgets_where = fgets_buf + fgets_empty;
	
	return size + tmp;
}
