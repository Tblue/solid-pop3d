static const char rcsid[] = "$Id: memops.c,v 1.1.1.1 2000/04/12 20:52:25 jurekb Exp $";
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

#ifndef STDC_HEADERS
#ifndef HAVE_MEMCPY
void *memchr(void *s, int c, size_t n) {
	size_t tmp;
	
	for (tmp = 0; tmp < n; tmp++)
		if (*((char *)(s + tmp)) == c)
			return (s + tmp);
	return (void *)0;
};
#endif
#endif
