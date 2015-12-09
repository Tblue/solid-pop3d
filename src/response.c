static const char rcsid[] = "$Id: response.c,v 1.3 2000/04/28 16:58:55 jurekb Exp $";
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

/* Code from this file sometimes works with root privileges */

#include "includes.h"
#include "response.h"

#include "const.h"
#include "log.h"

void send_error(const char *fmt,...)
{
	va_list args;
	char buf[MAXRESPLN];

	va_start(args, fmt);
	strcpy(buf, "-ERR ");
	vsnprintf(buf + 5, (sizeof(buf) - 5), fmt, args);
	va_end(args);
	strcat(buf, "\r\n");
	if (write(1, buf, strlen(buf)) <= 0) {
		pop_log(pop_priority, "send_error(): can't write to socket");
		pop_error("write");
		exit(1);
	};
}

void send_ok(const char *fmt,...)
{
	va_list args;
	char buf[MAXRESPLN];

	va_start(args, fmt);
	strcpy(buf, "+OK ");
	vsnprintf(buf + 4, (sizeof(buf) - 4), fmt, args);
	va_end(args);
	strcat(buf, "\r\n");
	if (write(1, buf, strlen(buf)) <= 0) {
		pop_log(pop_priority, "send_ok(): can't write to socket");
		pop_error("write");
		exit(1);
	};
}
