static const char rcsid[] = "$Id: log.c,v 1.3 2000/04/28 16:58:55 jurekb Exp $";
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
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include "log.h"

struct log_struct {
	char *name;
	int num;
};

const struct log_struct log_facilities[] = {
	{"daemon", LOG_DAEMON},
	{"local0", LOG_LOCAL0},
	{"local1", LOG_LOCAL1},
	{"local2", LOG_LOCAL2},
	{"local3", LOG_LOCAL3},
	{"local4", LOG_LOCAL4},
	{"local5", LOG_LOCAL5},
	{"local6", LOG_LOCAL6},
	{"local7", LOG_LOCAL7},
	{"mail", LOG_MAIL},
	{"user", LOG_USER},
	{NULL, 0}
};

const struct log_struct log_priorities[] = {
	{"emerg", LOG_EMERG},
	{"alert", LOG_ALERT},
	{"crit", LOG_CRIT},
	{"err", LOG_ERR},
	{"warning", LOG_WARNING},
	{"notice", LOG_NOTICE},
	{"info", LOG_INFO},
	{"debug", LOG_DEBUG},
	{NULL, 0}
};


int pop_debug = 0;
int pop_facility = POP_FACILITY;
int pop_priority = POP_PRIORITY;
char logpriority[64];

void pop_openlog(void) {
	openlog(POP_IDENT, LOG_PID, pop_facility);
}

void pop_closelog() {
	closelog();
}

void pop_log(int priority, const char *format, ...) {
	char tmp[1536];
	va_list lst;

	va_start(lst, format);
	vsnprintf(tmp, sizeof(tmp), format, lst);
	va_end(lst);
	syslog(priority, "%s", tmp);
}

void pop_log_dbg(int priority, const char *format, ...) {
	char tmp[1536];
	va_list lst;

	if (!pop_debug)
		return;
	va_start(lst, format);
	vsnprintf(tmp, sizeof(tmp), format, lst);
	va_end(lst);
	syslog(priority, "%s", tmp);
}


void pop_error(const char *name) {
	pop_log(pop_priority, "%.200s: %.200s", name, strerror(errno));
}

void pop_error_dbg(const char *name) {
	if (pop_debug)
		pop_error(name);
}

int search_log_struct(char *name, const struct log_struct table[]) {
	int i = 0;
	
	while ((table[i].name != NULL)) {
		if (strcasecmp(table[i].name, name) == 0)
			break;
		i++;
	};
	return (table[i].name == NULL) ? -1 : table[i].num;
}

int check_logpriority(void *priority) {
	char *tmp;
	int tmp_facility = pop_facility, tmp_priority = pop_priority;
	
	if ((tmp = strchr((char *) priority, '.')) != NULL) {
		*tmp = 0;
		tmp++;
	};
	if ((tmp_facility = search_log_struct((char *) priority, \
					      log_facilities)) < 0)
		return -1;
	if (tmp)
		if ((tmp_priority = search_log_struct(tmp, \
						      log_priorities)) < 0)
			return -1;
	pop_priority = tmp_priority;
	if (pop_facility != tmp_facility) {
		pop_facility = tmp_facility;
		pop_closelog();
		pop_openlog();
	};
	return 0;
}
