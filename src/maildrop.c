static const char rcsid[] = "$Id: maildrop.c,v 1.3 2000/04/28 16:58:55 jurekb Exp $";
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

#include "cmds.h"
#include "includes.h"
#include "const.h"
#include "maildrop.h"
#ifdef MDMAILBOX
#include "mailbox.h"
extern struct str_maildrop mb_maildrop;
#endif
#ifdef MDMAILDIR
#include "maildir.h"
extern struct str_maildrop mdir_maildrop;
#endif
#include "log.h"
#include "fdfgets.h"
#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>

extern void check_wccount(void);

struct message *messages = NULL;
char *specific = NULL;
int msgnr, msgdel = 0, maxmsgnr;
extern char maildrop_name[];
extern char username[];
struct str_maildrop_name maildrops[] =
{
#ifdef MDMAILBOX
	{"mailbox", &mb_maildrop},
#endif
#ifdef MDMAILDIR
	{"maildir", &mdir_maildrop},
#endif
	{NULL, NULL}
};


struct str_maildrop *find_maildrop(char *name) {
	int tmp = 0;
	
	while (maildrops[tmp].name != NULL)
		if (strcasecmp(maildrops[tmp++].name, name) == 0)
			break;
	return (strcasecmp(maildrops[tmp - 1].name, name) == 0) ? (maildrops[tmp - 1].mdrop) : NULL;
}


int md_alloc(int size) {
	int i;

	maxmsgnr = MSGNR_INCREMENT;
	msgnr = 0;

	if ((messages = calloc(maxmsgnr, sizeof(struct message))) == NULL)
		return -1;

	if ((specific = calloc(maxmsgnr, size)) == NULL)
		return -1;

	for (i = 0; i < maxmsgnr; i++) {
		messages[i].md_specific = specific + (i * size);
	}

	return 0;
}

int md_realloc(int size) {
	struct message *tmp;
	char *tmp2;
	int tmp3;

	if (msgnr < maxmsgnr)
		return 0;
	maxmsgnr += 100;
	if (maxmsgnr > MAXMSGNR)
		return -1;

	if ((tmp = (struct message *) realloc(messages, maxmsgnr * sizeof(struct message))) == NULL)
		return -1;
	memset(tmp + maxmsgnr - 100, 0, 100 * sizeof(struct message));

	messages = tmp;
	if ((tmp2 = ((char *)realloc(specific, maxmsgnr * size))) == NULL)
		return -1;
	specific = tmp2;
	for (tmp3 = 0; tmp3 < maxmsgnr; tmp3++) {
		messages[tmp3].md_specific = specific + (tmp3 * size);
		if (tmp3 >= msgnr)
			memset(messages[tmp3].md_specific, 0, size);
	};
	return 0;
}

void md_free(void) {
	if (messages)
		free(messages);
	if (specific)
		free(specific);
}

void md_stat(void) {
	int tmp, totalmsgs = msgnr - msgdel;
	size_t totalsize = 0;

	for (tmp = 0; tmp < msgnr; tmp++)
		if (!messages[tmp].deleted)
			totalsize += messages[tmp].crlfsize;
	send_ok("%u %u", totalmsgs, totalsize);
}

void md_reset(void)
{
	int tmp;

	for (tmp = 0; tmp < msgnr; tmp++)
		messages[tmp].deleted = 0;
	msgdel = 0;
	send_ok("all messages unmarked");
}

void md_delete(unsigned int nr)
{
	if (nr > msgnr) {
		send_error("no such message");
		check_wccount();
		return;
	};
	if (messages[--nr].deleted) {
		send_error("message %u already marked as deleted", nr + 1);
		check_wccount();
		return;
	};
	messages[nr].deleted = 1;
	msgdel++;
	send_ok("message %u marked as deleted", nr + 1);
}

char *md_print_uidl(unsigned int nr, _md_md5_uidl_message md5_uidl_message, char *result) {
	static char digits[16]="0123456789abcdef";
	char digest[16];
	int tmp;	
	
	md5_uidl_message(nr, digest);
	for (tmp = 0; tmp < 16; tmp++) {
		result[tmp*2] = digits[(digest[tmp] >> 4) & 0x0F];
		result[(tmp*2) + 1] = digits[digest[tmp] & 0x0F];
	};
	result[32] = 0;
	return result;
}

void md_uidl(unsigned int nr, _md_md5_uidl_message md5_uidl_message) {
	int tmp;
	char result[33];
	char mbuf[128];
	
	if (nr > msgnr) {
		send_error("no such message");
		check_wccount();
		return;
	};
	if (nr == 0) {
		send_ok("");
		for (tmp = 0; tmp < msgnr; tmp++) {
			if (messages[tmp].deleted)
				continue;
			snprintf(mbuf, sizeof(mbuf), "%u %.33s\r\n", tmp + 1, md_print_uidl(tmp, md5_uidl_message, result));
			if (write(1, mbuf, strlen(mbuf)) != strlen(mbuf)) {
				pop_log(pop_priority, "maildrop: can't write to socket");
				pop_error("maildrop: write");
				exit(1);
			};
		};
		if (write(1, ".\r\n", 3) != 3) {
			pop_log(pop_priority, "maildrop: can't write to socket");
			pop_error("maildrop: write");
			exit(1);
		};
		return;
	};
	if (messages[--nr].deleted) {
		send_error("message %u is marked as deleted", nr + 1);
		check_wccount();
		return;
	};
	send_ok("%u %.33s", nr + 1, md_print_uidl(nr, md5_uidl_message, result));
}

void md_list(unsigned int nr) {
	int tmp;
	char mbuf[128];

	if (nr > msgnr) {
		send_error("no such message");
		check_wccount();
		return;
	};
	if (nr == 0) {
		send_ok("scan listing follows");
		for (tmp = 0; tmp < msgnr; tmp++) {
			if (messages[tmp].deleted)
				continue;
			snprintf(mbuf, sizeof(mbuf), "%u %u\r\n", tmp + 1, messages[tmp].crlfsize);
			if (write(1, mbuf, strlen(mbuf)) != strlen(mbuf)) {
				pop_log(pop_priority, "maildrop: can't write to socket");
				pop_error("maildrop: write");
				exit(1);
			};
		};
		if (write(1, ".\r\n", 3) != 3) {
			pop_log(pop_priority, "maildrop: can't write to socket");
			pop_error("maildrop: write");
			exit(1);
		};
		return;
	};
	if (messages[--nr].deleted) {
		send_error("message %u is marked as deleted", nr + 1);
		check_wccount();
		return;
	};
	send_ok("%u %u", nr + 1, messages[nr].crlfsize);
}

void md_top(unsigned int nr, unsigned int lcount, int fd, _md_cleanup cleanup) {
	ssize_t size, tmp;
	int newline;
	char mbuf[128];

	size = messages[nr].size;
	if (messages[nr].missing_eol) {
		/* Account for missing ("phantom") trailing newline in message.
		 * We will add it later.
		 */
		size--;
	}

/* send header */
	fd_initfgets();
	newline = 1;
	send_ok("");
	while (((tmp = fd_fgets(mbuf, sizeof(mbuf), fd)) > 0) && (size > 0)) {
		size -= tmp;
		if (size < 0) {
			cleanup();
			pop_log(pop_priority, "maildrop: maildrop content has been changed");
			exit(1);
		};		
		if ((newline) && mbuf[0] == '.')
			if (write(1, ".", 1) != 1) {
				cleanup();
				pop_log(pop_priority, "maildrop: can't write to socket");
				pop_error("maildrop: write");
				exit(1);
			};
		if ((newline) && (tmp == 1) && mbuf[0] == '\n')			
			break;		
		if (mbuf[tmp - 1] == '\n') {
			newline = 1;
			if (write(1, mbuf, tmp - 1) != (tmp - 1)) {
				cleanup();
				pop_log(pop_priority, "maildrop: can't write to socket");
				pop_error("maildrop: write");
				exit(1);
			};
			if (write(1, "\r\n", 2) != 2) {
				cleanup();
				pop_log(pop_priority, "maildrop: can't write to socket");
				pop_error("maildrop: write");
				exit(1);
			};
		} else {
			if (write(1, mbuf, tmp) != tmp) {
				cleanup();
				pop_log(pop_priority, "maildrop: can't write to socket");
				pop_error("maildrop: write");
				exit(1);
			};
			newline = 0;
		};		
	};
	if (tmp < 0) {
		cleanup();
		pop_log(pop_priority, "maildrop: can't read message");
		pop_error("maildrop: read");
		exit(1);
	};	
	newline = 1;
	if (write(1, "\r\n", 2) != 2) {
		cleanup();
		pop_log(pop_priority, "maildrop: can't write to socket");
		pop_error("maildrop: write");
		exit(1);
	};
/* First lcount lines of message */
	while (((tmp = fd_fgets(mbuf, sizeof(mbuf), fd)) > 0) && (size > 0) && (lcount > 0)) {
		size -= tmp;
		if (size < 0) {
			cleanup();
			pop_log(pop_priority, "maildrop: maildrop content has been changed");
			exit(1);
		};
		if ((newline) && mbuf[0] == '.')
			if (write(1, ".", 1) != 1) {
				cleanup();
				pop_log(pop_priority, "maildrop: can't write to socket");
				pop_error("maildrop: write");
				exit(1);
			};
		if (mbuf[tmp - 1] == '\n') {
			lcount--;
			newline = 1;
			if (write(1, mbuf, tmp - 1) != (tmp - 1)) {
				cleanup();
				pop_log(pop_priority, "maildrop: can't write to socket");
				pop_error("maildrop: write");
				exit(1);
			};
			if (write(1, "\r\n", 2) != 2) {
				cleanup();
				pop_log(pop_priority, "maildrop: can't write to socket");
				pop_error("maildrop: write");
				exit(1);			
			};
		} else {
			if (write(1, mbuf, tmp) != tmp) {
				cleanup();
				pop_log(pop_priority, "maildrop: can't write to socket");
				pop_error("maildrop: write");
				exit(1);
			};
			newline = 0;
		};		
	};

	if (tmp < 0) {
		cleanup();
		pop_log(pop_priority, "maildrop: can't read message");
		pop_error("maildrop: read");
		exit(1);
	};

	if (size == 0 && messages[nr].missing_eol) {
		/* We output the entire message, add the missing trailing newline characters. */
		if (write(1, "\r\n", 2) != 2) {
			cleanup();
			pop_log(pop_priority, "maildrop: can't write to socket");
			pop_error("maildrop: write");
			exit(1);
		}
	}
}

void md_retrieve(unsigned int nr, int fd, _md_cleanup cleanup) {
	ssize_t size, mcount;
	int newline;
	char mbuf[128];
	
	messages[nr].cread = 1;
	send_ok("%u octets", messages[nr].crlfsize);

	size = messages[nr].size;
	if (messages[nr].missing_eol) {
		/* Message has no trailing newline character, so we cheat and just
		 * add one after the normal message content has been sent to the
		 * client. Need to decrement the message size, though, since we do
		 * need the real message size in the loop below and that "phantom
		 * newline" has already been included in the message size at the time
		 * the message was parsed to make other POP3 commands display correct
		 * octet counts (i. e. so that these counts match what we output here).
		 */
		size--;
	}

	newline = 1;
	fd_initfgets();
	while (size > 0) {
		if ((mcount = fd_fgets(mbuf, sizeof(mbuf), fd)) <= 0) {
			cleanup();
			pop_log(pop_priority, "maildrop: can't read message");
			if (mcount < 0)
				pop_error("maildrop: read");
			exit(1);
		};
		size -= mcount;
		if ((newline) && (mbuf[0] == '.'))
			if (write(1, ".", 1) != 1) {
				cleanup();
				pop_log(pop_priority, "maildrop: can't write to socket");
				pop_error("maildrop: write");
				exit(1);
			};
		if ((mbuf[mcount - 1]) == '\n') {
			newline = 1;			
			if (write(1, mbuf, mcount - 1) != (mcount - 1)) {
				cleanup();
				pop_log(pop_priority, "maildrop: can't write to socket");
				pop_error("maildrop: write");
				exit(1);
			};
			if (write(1, "\r\n", 2) != 2) {
				cleanup();
				pop_log(pop_priority, "maildrop: can't write to socket");
				pop_error("maildrop: write");
				exit(1);
			};
		} else {
			newline = 0;
			if (write(1, mbuf, mcount) != mcount) {
				cleanup();
				pop_log(pop_priority, "maildrop: can't write to socket");
				pop_error("maildrop: write");
				exit(1);
			};
		};
	};

	if (size != 0) {
		cleanup();
		pop_log(pop_priority, "maildrop: maildrop content has been changed");
		exit(1);
	};

	if (messages[nr].missing_eol) {
		/* Add the missing trailing newline characters. */
		if (write(1, "\r\n", 2) != 2) {
			cleanup();
			pop_log(pop_priority, "maildrop: can't write to socket");
			pop_error("maildrop: write");
			exit(1);
		}
	}
}

void md_end_reply(_md_cleanup cleanup) {
	if (write(1, ".\r\n", 3) != 3) {
		cleanup();
		pop_log(pop_priority, "maildrop: can't write to socket");
		pop_error("maildrop: write");
		exit(1);
	};
}

/* vim: set ts=4 sw=4 noet: */
