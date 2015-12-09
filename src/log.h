/* $Id: log.h,v 1.2 2000/04/17 19:38:03 jurekb Exp $ */
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

#ifndef _LOG_H
#define _LOG_H

#include <syslog.h>
#include "includes.h"

void pop_openlog(void);
void pop_closelog(void);
void pop_log(int priority, const char *format, ...);
void pop_log_dbg(int priority, const char *format, ...);
void pop_error(const char *name);
void pop_error_dbg(const char *name);

extern int pop_priority;
int check_logpriority(void *name);

#endif /* log.h */
