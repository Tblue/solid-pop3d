/* $Id: const.h,v 1.1.1.1 2000/04/12 20:52:26 jurekb Exp $ */
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

#ifndef _CONST_H
#define _CONST_H

#include "includes.h"

#define MAXRESPLN 512
#define MAXCMDLN 255
#define MAXARGLN 40
#define MAXPREFIX 16

#define AUTH_STATE 0
#define TRANSACTION_STATE 1
#define SERVICE_NAME "solid-pop3d"
#define MSGNR_INCREMENT 100
#define MAXMSGNR 10000
#define SERVER_GREETING "Solid POP3 server ready"
#define DEFAUTOLOGOUTTIME 60
#define DEFWCCOUNT 5

#ifdef MDMAILBOX
#define DEFMAILDROPNAME "/var/mail/%s"
#else
#define DEFMAILDROPNAME "Maildir"
#endif

#ifdef MDMAILBOX
#define DEFMAILDROPTYPE "mailbox"
#else
#define DEFMAILDROPTYPE "maildir"
#endif

#define MAXMDTYPENAMELENGTH 40
#define USERCFG ".spop3d"
#define USERBULL ".spop3d-bull"
#define POPUSER "spop3d"

#define PER_SOURCE 5
#define MAX_SESSIONS 256
#define SOCKET_SYN_QUEUE 5
#define MIN_DELAY 50
#define POP3_PORT 110

#define POP_IDENT "solid-pop3d"
#define POP_PRIORITY LOG_NOTICE
#define POP_FACILITY LOG_LOCAL0
#endif /* const.h */
