/* $Id: includes.h,v 1.2 2000/04/28 16:58:55 jurekb Exp $ */
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

#ifndef _INCLUDES_H
#define _INCLUDES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#else /* HAVE_CONFIG_H */
#error "config.h file required. run ./configure first"
#endif /* HAVE_CONFIG_H */

#include "const.h"

#ifndef MDMAILBOX
#ifndef MDMAILDIR
#error "you must compile server with support for at least one maildrop type"
#endif
#endif

#ifdef APOP
#ifndef USERCONFIG
#error "you must compile server with user configuration support if you want to use APOP"
#endif
#endif

#include <sys/types.h>
#include <stdio.h>

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#else /* STDC_HEADERS */
#include <stdarg.h>
#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */
char *strchr(), *strrchr();
#ifndef HAVE_MEMCPY
#define memcpy(d, s, n) bcopy((s), (d), (n))
#define memmove(d, s, n) bcopy((s), (d), (n))
#define memset(d, ch, n) bzero((d), (n)) /* We use memset() only to zeroing. */
#define memcmp(a, b, n) bcmp((a), (b), (n))
void *memchr(void *s, int c, size_t n);
#endif /* HAVE_MEMCPY */
#endif /* STDC_HEADERS */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif /* HAVE_LIMITS_H */
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif /* PATH_MAX */
#include <ctype.h>
#endif /* includes.h */
