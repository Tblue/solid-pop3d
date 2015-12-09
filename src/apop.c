static const char rcsid[] = "$Id: apop.c,v 1.2 2000/04/28 16:58:55 jurekb Exp $";
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
#include "apop.h"
#include "const.h"
#include "md5.h"
#include "log.h"
#include <sys/stat.h>
#include <fcntl.h>

extern char apop_secret[];

int apop_authenticate(char *username, char *apoptimestamp, char *udigest) {
	char adigest[16], digest[16];
	struct md5_ctx context;
	int tmp;
	
	for (tmp = 0; tmp < 16; tmp++) {
		if (isdigit(udigest[tmp * 2]))
			digest[tmp] = ((udigest[tmp * 2] - '0') << 4);
		else
			digest[tmp] = ((udigest[tmp * 2] - 'a' + 10) << 4);
		if (isdigit(udigest[(tmp * 2) + 1]))
			digest[tmp] |= (udigest[(tmp * 2) + 1] - '0');
		else
			digest[tmp] |= (udigest[(tmp * 2) + 1] - 'a' + 10);
	};
	md5_init_ctx(&context);
	md5_process_bytes(apoptimestamp, strlen(apoptimestamp), &context);
	md5_process_bytes(apop_secret, strlen(apop_secret), &context);
	md5_finish_ctx(&context, &adigest);
	return (memcmp(digest, adigest, 16) == 0) ? 0 : -1;
}
