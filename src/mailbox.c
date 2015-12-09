static const char rcsid[] = "$Id: mailbox.c,v 1.6 2000/05/13 13:25:52 jurekb Exp $";
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
#include "maildrop.h"
#include "mailbox.h"
#include "const.h"
#include "md5.h"
#include "log.h"
#include "fdfgets.h"

#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#ifdef MAILOCK
#include <maillock.h>
#endif
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif


extern void check_wccount(void);
extern char maildrop_name[];
extern char username[];
int mailboxfd;
off_t mailboxsize, lastmailboxsize;
time_t mailboxmtime;
#ifdef CREATEMAILDROP
extern int createmaildrop;
#endif
extern int msgnr, maxmsgnr, msgdel;
extern struct message *messages;
char months[][3] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "xxx"};

struct str_maildrop mb_maildrop =
{
	mb_init,
	mb_update,
	mb_release,
	mb_retrieve,
	mb_top,
#ifndef BULLETINS
	mb_md5_uidl_message
#else
	mb_md5_uidl_message,
	mb_add_message,
	mb_end_of_adding
#endif
};

int unlock_mailbox(void) {
	int retcode;
#ifdef HAVE_FLOCK
#define MLNAME "mailbox: flock"
	retcode = flock(mailboxfd, LOCK_UN);
#else
#if defined(F_SETLK) && defined(F_SETLKW)
#define MLNAME "mailbox: fcntl"
	struct flock arg;
	
	arg.l_type = F_UNLCK;
	arg.l_whence = arg.l_start = arg.l_len = arg.l_pid = 0;	
	retcode = fcntl(mailboxfd, F_SETLKW, &arg);
#else
#define MLNAME "mailbox: lockf"
	retcode = lockf(mailboxfd, F_ULOCK, 0);
#endif /* defined(F_SETLK) && defined(F_SETLKW) */
#endif /* HAVE_FLOCK */
#ifdef MAILOCK
	mailunlock();
#endif
	return retcode;
}

int lock_mailbox(void) {
#if defined(F_SETLK) && defined(F_SETLKW) && !defined(HAVE_FLOCK)
	struct flock arg;
#endif

#ifdef MAILOCK
	if (maillock(username, 1) != 0) {
		pop_error("mailbox: maillock");
		return -1;
	};
#endif

#ifdef HAVE_FLOCK
	return flock(mailboxfd, LOCK_EX);
#else
#if defined(F_SETLK) && defined(F_SETLKW)
	arg.l_type = F_WRLCK;
	arg.l_whence = arg.l_start = arg.l_len = arg.l_pid = 0;	
	return fcntl(mailboxfd, F_SETLKW, &arg);
#else
	return lockf(mailboxfd, F_LOCK, 0);
#endif /* defined(F_SETLK) && defined(F_SETLKW) */
#endif /* HAVE_FLOCK */
}

long int mb_dec(char *in) {	
	long int value;
	
	value = strtol(in, NULL, 10);
	return (((value == LONG_MIN) || (value == LONG_MAX)) ? -1 : value);
}

time_t mb_readtime(char *in) {
	int m;
	struct tm vtm;	
	
	for (m = 0; m < 13; m++)
		if (strncmp(in, months[m], 3) == 0)
			break;
	if ((m == 12) || (in[3] != ' '))
		return -1;
	vtm.tm_mon = m;	

	vtm.tm_mday = mb_dec(in += 4);
	if (in[2] != ' ')
		return -1;	

	vtm.tm_hour = mb_dec(in += 3);	
	if (in[2] != ':')
		return -1;

	vtm.tm_min = mb_dec(in += 3);
	if (in[2] != ':')
		return -1;

	vtm.tm_sec = mb_dec(in += 3);
	if (in[2] != ' ')
		return -1;
	
	vtm.tm_year = mb_dec(in += 3) - 1900;
	vtm.tm_isdst = -1;
	return mktime(&vtm);
}

int mb_compare(const char *a, const char *b, int alength) {
	if (alength < strlen(b))
		return 0;
	return (strncmp(a, b, strlen(b)) == 0) ? 1 : 0;
}

int mb_fixed(const char *what, int alength) {
	return (mb_compare(what, "Received:", alength) ||
		mb_compare(what, "Date:", alength) ||
		mb_compare(what, "Message-Id:", alength) ||
		mb_compare(what, "Subject:", alength));
}


#ifdef BULLETINS

void mb_end_of_adding(void) {
	struct stat stbuf;

	if (mailboxfd < 0)
		return;
	if (fstat(mailboxfd, &stbuf) < 0) {
		unlock_mailbox();
		pop_error("mailbox: fstat");
		send_error("fatal error");
		exit(1);
	};
	if (unlock_mailbox() < 0) {
		pop_error(MLNAME);
		send_error("fatal error");
		exit(1);
	};
	mailboxmtime = stbuf.st_mtime;
	lastmailboxsize = mailboxsize = stbuf.st_size;
}


int mb_add_message(int fd) {
	off_t tmpwhere;
	ssize_t mcount, tmp;
	char linebuf[128];
	char msgdate[21];
	struct mb_message *mbspecific;
	struct md5_ctx context;
	int header = 1, fixed = 0, newline = 1;
	
	if (mailboxfd < 0) {
		pop_log(pop_priority, "mailbox: can't add bulletin to mailbox - \
mailbox doesn't exist");
		return 0;
	}
	if ((tmpwhere = lseek(mailboxfd, 0, SEEK_END)) < 0) {
		unlock_mailbox();
		close(fd);
		pop_error("mailbox: lseek");
		send_error("fatal error");
		exit(1);
	};
	fd_initfgets();
	if ((mcount = fd_fgets(linebuf, sizeof(linebuf), fd)) < 0) {
		unlock_mailbox();
		close(fd);
		pop_log(pop_priority, "mailbox: can't read message");
		pop_error("mailbox: read");
		send_error("fatal error");
		exit(1);	
	};
	if (mcount < (5 + 2 + sizeof(msgdate))) { /* "From " + username + DATE */
		unlock_mailbox();
		close(fd);
		pop_log(pop_priority, "mailbox: message is too short");
		send_error("fatal error");
		exit(1);
	};
	if (strncmp(linebuf, "From ", 5) != 0) {
		unlock_mailbox();
		close(fd);
		pop_log(pop_priority, "mailbox: missing \"From \"");
		send_error("fatal error");
		exit(1);
	};
	if (md_realloc(sizeof(struct mb_message)) < 0) {
		unlock_mailbox();
		close(fd);
		pop_log(pop_priority, "mailbox: no memory available");
		send_error("no memory available");
		exit(1);
	};
	msgnr++;
	messages[msgnr - 1].deleted = messages[msgnr - 1].crlfsize = messages[msgnr - 1].size = 0;
	mbspecific = ((struct mb_message *)(messages[msgnr - 1].md_specific));
	mbspecific->from_where = tmpwhere;
	mbspecific->from_size = 0;
	while (mcount > 0) {		
		if (mcount < (sizeof(msgdate))) {
			memmove(msgdate, msgdate + mcount, (sizeof(msgdate) - mcount));
			memcpy(msgdate + (sizeof(msgdate) - mcount), linebuf, mcount);
		} else
			memcpy(msgdate, linebuf + mcount - sizeof(msgdate), sizeof(msgdate));

		tmpwhere += mcount;
		mbspecific->from_size += mcount;
		if (write(mailboxfd, linebuf, mcount) != mcount) {
			unlock_mailbox();
			close(fd);
			pop_log(pop_priority, "mailbox: can't write to mailbox: %.1024s", maildrop_name);
			pop_error("mailbox: write");
			send_error("fatal error");
			exit(1);
		};		
		if (linebuf[mcount - 1] == '\n')
			break;
		mcount = fd_fgets(linebuf, sizeof(linebuf), fd);
	};
	if (mcount < 0) {
		unlock_mailbox();
		close(fd);
		pop_log(pop_priority, "mailbox: can't read from message file");
		pop_error("mailbox: read");
		send_error("message file is damaged");
		exit(1);
	};
	if (msgdate[sizeof(msgdate) - 1] != '\n') {
		unlock_mailbox();
		close(fd);
		pop_log(pop_priority, "mailbox: message file is damaged");
		send_error("can't read from message file");
		exit(1);
	};
	msgdate[sizeof(msgdate) - 1] = 0;
	messages[msgnr - 1].msg_time = mb_readtime(msgdate);
	mbspecific->where = tmpwhere;	
	md5_init_ctx(&context);
	while ((mcount = fd_fgets(linebuf, sizeof(linebuf), fd)) > 0) {
		if (write(mailboxfd, linebuf, mcount) != mcount) {
			unlock_mailbox();
			close(fd);
			pop_log(pop_priority, "mailbox: can't write to mailbox: %.1024s", maildrop_name);
			pop_error("mailbox: write");
			send_error("fatal error");
			exit(1);
		};
		if (header && newline)
			fixed = mb_fixed(linebuf, mcount);
		if (fixed)
			md5_process_bytes(linebuf, mcount, &context);
		messages[msgnr - 1].crlfsize += mcount;
		messages[msgnr - 1].size += mcount;
		mbspecific->from_size += mcount;
		if (linebuf[mcount - 1] == '\n') {
			messages[msgnr - 1].crlfsize++;
			if ((mcount == 1) && newline)
				header = 0;
			newline = 1;
		}
		tmp = mcount;
	};
	md5_finish_ctx(&context, mbspecific->digest);
	close(fd);
	if (mcount < 0) {
		unlock_mailbox();
		pop_log(pop_priority, "mailbox: can't read from message file");
		pop_error("mailbox: read");
		send_error("can't read from message file");
		exit(1);
	};
	return 1;
}
#endif


int mb_parse(int compare) {
	ssize_t tmp2;
	char mbuf[128];
	ssize_t mcount;
	off_t act_ofs = 0;
	char msgdate[21];
	struct mb_message *tmp;
	struct md5_ctx context;
	int tmpmsgnr = 0;
	time_t tmpmsg_time;
	off_t tmpfrom_where = 0, tmpwhere = 0;
	size_t tmpfrom_size = 0, tmpsize = 0, tmpcrlfsize = 0;
	char tmpdigest[16];
	int newline = 1, header, fixed;
	
	if (compare)
		if (lseek(mailboxfd, 0, SEEK_SET) < 0) {
			pop_error("mailbox: lseek");
			return -1;		
		};
/* parse mailbox */
	if (!compare)
		if (md_alloc(sizeof(struct mb_message)) < 0) {
			unlock_mailbox();
			mb_release();
			pop_log(pop_priority, "mailbox: no memory available");
			send_error("no memory available");
			exit(1);
		};
	fd_initfgets();
	if ((mcount = fd_fgets(mbuf, sizeof(mbuf), mailboxfd)) < 0) {		
		if (!compare) {
			unlock_mailbox();
			mb_release();
		};
		pop_log(pop_priority, "mailbox: mailbox %.1024s is damaged", maildrop_name);
		pop_error("mailbox: read");
		if (!compare) {
			send_error("mailbox is damaged");
			exit(1);
		};
		return -1;
	};
	if (mcount == 0) {
		if (!compare) {
			msgnr = 0;
			return 0;
		} else
			return ((msgnr == 0) ? 0 : -1);
	};
	if ((mcount > 0) && (mcount < (5 + 2 + sizeof(msgdate)))) {
		if (!compare) {
			unlock_mailbox();
			mb_release();
		};
		pop_log(pop_priority, "mailbox: mailbox %.1024s is damaged", maildrop_name);
		if (!compare) {
			send_error("mailbox is damaged");
			exit(1);
		};
		return -1;
	};
	if ((mcount > 0) && (strncmp(mbuf, "From ", 5) != 0)) {
		if (!compare) {
			unlock_mailbox();
			mb_release();
		};
		pop_log(pop_priority, "mailbox: mailbox %.1024s is damaged", maildrop_name);
		if (!compare) {
			send_error("mailbox is damaged");
			exit(1);
		};
		return -1;
	};
	tmp2 = mcount;	
	while (mcount > 0) {
		act_ofs += mcount;
		if ((newline) && (mcount >= (5 + 2 + sizeof(msgdate))) && (strncmp(mbuf, "From ", 5) == 0)) {
			header = 1; fixed = 0;
			if (!compare)
				messages[tmpmsgnr].deleted = messages[tmpmsgnr].cread = 0;
			if (tmpmsgnr > 0) {
					tmp = (struct mb_message *)(messages[tmpmsgnr - 1].md_specific);
					md5_finish_ctx(&context, tmpdigest);
				if (!compare) {
					tmp->from_where = tmpfrom_where;
					messages[tmpmsgnr - 1].size = tmpsize;
					messages[tmpmsgnr - 1].crlfsize = tmpcrlfsize;
					tmp->where = tmpwhere;
					tmp->from_size = tmpfrom_size;
					memcpy(tmp->digest, tmpdigest, 16);
					messages[tmpmsgnr - 1].msg_time = tmpmsg_time;
				} else {
					if ((tmp->from_where != tmpfrom_where) ||
					    (messages[tmpmsgnr - 1].size != tmpsize) ||
					    (messages[tmpmsgnr - 1].crlfsize != tmpcrlfsize) ||
					    (tmp->where != tmpwhere) ||
					    (tmp->from_size != tmpfrom_size) ||
					    (memcmp(tmp->digest, tmpdigest, 16) != 0) ||
					    (messages[tmpmsgnr - 1].msg_time != tmpmsg_time))
						    return -1;
				};
			};
			if (compare && (tmpmsgnr >= msgnr))
				return 0; /* messages added only */
			tmpfrom_where = act_ofs - mcount;
			tmpmsgnr++;
			if (!compare)
				msgnr = tmpmsgnr;
			if (!compare)
				if (md_realloc(sizeof(struct mb_message)) < 0) {
					unlock_mailbox();
					mb_release();
					pop_log(pop_priority, "mailbox: no memory available");
					send_error("no memory available");
					exit(1);
				};
			md5_init_ctx(&context);
			tmpfrom_size = mcount;
			if (mcount < (sizeof(msgdate))) {
				memmove(msgdate, msgdate + mcount, (sizeof(msgdate) - mcount));
				memcpy(msgdate + (sizeof(msgdate) - mcount), mbuf, mcount);
			} else
				memcpy(msgdate, mbuf + mcount - sizeof(msgdate), sizeof(msgdate));
			while (mbuf[mcount - 1] != '\n') {
				mcount = fd_fgets(mbuf, sizeof(mbuf), mailboxfd);
				if (mcount <= 0)
				    break;
				if (mcount < (sizeof(msgdate))) {
					memmove(msgdate, msgdate + mcount, (sizeof(msgdate) - mcount));
					memcpy(msgdate + (sizeof(msgdate) - mcount), mbuf, mcount);
				} else
					memcpy(msgdate, mbuf + mcount - sizeof(msgdate), sizeof(msgdate));
				act_ofs += mcount;
				tmpfrom_size += mcount;
				
			};
			
			msgdate[sizeof(msgdate) - 1] = 0;
			tmpmsg_time = mb_readtime(msgdate);
			if (mcount <= 0) {
				if (!compare) {
					unlock_mailbox();
					mb_release();
				};
				pop_log(pop_priority, "mailbox: mailbox is damaged: %.1024s", maildrop_name);
				if (!compare) {
					send_error("mailbox is damaged");
					exit(1);
				};
				return -1;
			};
			newline = 1;
			tmpwhere = act_ofs;		
			tmpsize = tmpcrlfsize = 0;
		} else {
			tmpsize += mcount;
			tmpcrlfsize += mcount;
			tmpfrom_size += mcount;
			if (header && newline)
				fixed = mb_fixed(mbuf, mcount);
			if (fixed)
				md5_process_bytes(mbuf, mcount, &context);
			if (mbuf[mcount - 1] == '\n') {
				tmpcrlfsize++;
				if (newline && (mcount == 1))
					header = 0;
				newline = 1;
			} else
				newline = 0;
		};
		tmp2 = mcount;
		mcount = fd_fgets(mbuf, sizeof(mbuf), mailboxfd);		
	};
	if (mcount < 0) {
		if (!compare) {
			unlock_mailbox();
			mb_release();
		};
		pop_log(pop_priority, "mailbox: mailbox is damaged: %.1024s", maildrop_name);
		pop_error("mailbox: read");
		if (!compare) {
			send_error("mailbox is damaged");
			exit(1);
		};
		return -1;
	};
	if (tmp2 > 0)
		if (mbuf[tmp2 - 1] != '\n') {		
			if (!compare) {
				unlock_mailbox();
				mb_release();
			};
			pop_log(pop_priority, "mailbox: mailbox is damaged: %.1024s", maildrop_name);			
			if (!compare) {
				send_error("mailbox is damaged");
				exit(1);
			};
			return -1;
		};
	tmp = (struct mb_message *)(messages[tmpmsgnr - 1].md_specific);
	md5_finish_ctx(&context, tmpdigest);
	if (!compare) {	
		messages[tmpmsgnr - 1].deleted = messages[tmpmsgnr - 1].cread = 0;
		tmp->from_where = tmpfrom_where;
		messages[tmpmsgnr - 1].size = tmpsize;
		messages[tmpmsgnr - 1].crlfsize = tmpcrlfsize;
		tmp->where = tmpwhere;
		tmp->from_size = tmpfrom_size;
		memcpy(tmp->digest, tmpdigest, 16);
		msgnr = tmpmsgnr;
		messages[tmpmsgnr - 1].msg_time = tmpmsg_time;
	} else {
		if ((tmp->from_where != tmpfrom_where) ||
		    (messages[tmpmsgnr - 1].size != tmpsize) ||
		    (messages[tmpmsgnr - 1].crlfsize != tmpcrlfsize) ||
		    (tmp->where != tmpwhere) ||
		    (tmp->from_size != tmpfrom_size) ||
		    (memcmp(tmp->digest, tmpdigest, 16) != 0) ||
		    (messages[tmpmsgnr - 1].msg_time != tmpmsg_time))
			    return -1;		
	};	
	return 0;
}

void mb_init(void) {	
	struct stat stbuf;
	
	msgnr = 0;
	if ((mailboxfd = open(maildrop_name, O_RDWR)) < 0) {
#ifdef CREATEMAILDROP
		if ((errno == ENOENT) && (createmaildrop))
			mailboxfd = open(maildrop_name, O_RDWR | O_CREAT | O_EXCL, 0600);
		if ((mailboxfd < 0) && (createmaildrop)) {
			pop_log(pop_priority, "mailbox: can't open mailbox file: %.1024s", maildrop_name);
			pop_error("mailbox: open");
			send_error("can't open mailbox file");
			exit(1);
		};
#endif
		if ((mailboxfd < 0) && (errno != ENOENT)) {
			pop_log(pop_priority, "mailbox: can't open mailbox file: %.1024s", maildrop_name);
			pop_error("mailbox: open");
			send_error("can't open mailbox file");
			exit(1);
		};
	};
	if (mailboxfd > 0) {
		if (lock_mailbox() < 0) {
			pop_log(pop_priority, "mailbox: can't set lock on mailbox file: %.1024s", maildrop_name);
			pop_error(MLNAME);
			send_error("can't lock mailbox file");
			exit(1);
		};
		if (fstat(mailboxfd, &stbuf) < 0) {
			unlock_mailbox();
			pop_error("mailbox: fstat");
			send_error("fstat failed");
			exit(1);
		};
		if (!S_ISREG(stbuf.st_mode)) {
			unlock_mailbox();
			pop_log(pop_priority, "mailbox: mailbox is not regular file: %.1024s", maildrop_name);
			send_error("mailbox is not regular file");
			exit(1);
		};
	}
#ifndef BULLETINS
	mailboxmtime = stbuf.st_mtime;
	lastmailboxsize = mailboxsize = stbuf.st_size;
#endif
	if (mailboxfd > 0)
		mb_parse(0);
#ifndef BULLETINS
	if (mailboxfd > 0)
		if (unlock_mailbox() < 0) {
			pop_log(pop_priority, "mailbox: can't remove lock from mailbox file: %.1024s", maildrop_name);
			pop_error(MLNAME);
			send_error("can't unlock mailbox file");
			exit(1);
		};
#endif
}

void mb_release(void)
{
	if (mailboxfd > 0)
		close(mailboxfd);
	md_free();
}

void mb_reparse(void) {
	struct stat stbuf;
	
	if (mailboxfd < 0)
		return;
	if (fstat(mailboxfd, &stbuf) < 0) {
		pop_error("mailbox: fstat");
		exit(1);
	};
	if ((stbuf.st_size != lastmailboxsize) ||
	    (stbuf.st_mtime != mailboxmtime)) {
		if (lock_mailbox() < 0) {
			pop_log(pop_priority, "mailbox: can't set lock on mailbox file: %.1024s", maildrop_name);
			pop_error(MLNAME);
			exit(1);
		};
		if (fstat(mailboxfd, &stbuf) < 0) {
			unlock_mailbox();
			pop_error("mailbox: fstat");
			exit(1);
		};
		if (stbuf.st_size < mailboxsize) {
			unlock_mailbox();
			pop_log(pop_priority, "mailbox: mailbox content has been changed");
			exit(1);
		};				
		if (mb_parse(1) < 0) {
			unlock_mailbox();
			pop_log(pop_priority, "mailbox: mailbox content has been changed");
			exit(1);
		};
		lastmailboxsize = stbuf.st_size;
		mailboxmtime = stbuf.st_mtime;
		if (unlock_mailbox() < 0) {
			pop_log(pop_priority, "mailbox: can't remove lock from mailbox file: %.1024s", maildrop_name);
			pop_error(MLNAME);
			exit(1);
		};
	};
}

void mb_cleanup(void) {
	if (mailboxfd > 0)
		unlock_mailbox();
}

void mb_retrieve(unsigned int nr) {
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
	mb_reparse();
	if (lseek(mailboxfd, ((struct mb_message *) (messages[nr].md_specific))->where, SEEK_SET) < 0) {
		unlock_mailbox();
		pop_error("mailbox: lseek");
		exit(1);
	};
	md_retrieve(nr, mailboxfd, &mb_cleanup);
	mb_reparse();
	md_end_reply(&mb_cleanup);
}

void mb_copy(off_t ofs, size_t length) {
	char mb_buf[10240];
	ssize_t tmp;
	off_t curofs;
	
	if ((curofs = lseek(mailboxfd, 0, SEEK_CUR)) < 0) {
		unlock_mailbox();
		pop_error("mailbox: lseek");
		send_error("fatal error");
		exit(1);
	};
	if (curofs == ofs) {
		if (lseek(mailboxfd, ofs + length, SEEK_SET) < 0) {
			unlock_mailbox();
			pop_error("mailbox: lseek");
			send_error("fatal error");
			exit(1);
		};
		return;
	};
	while (length > 0) {
		if (length > sizeof(mb_buf)) {
			length -= sizeof(mb_buf);
			tmp = sizeof(mb_buf);
		} else {
			tmp = length;
			length = 0;
		};
		if (lseek(mailboxfd, ofs, SEEK_SET) < 0) {
			unlock_mailbox();
			pop_error("mailbox: lseek");
			send_error("fatal error");
			exit(1);
		};
		if (read(mailboxfd, mb_buf, tmp) != tmp) {
			unlock_mailbox();
			pop_error("mailbox: read");
			send_error("read error");
			exit(1);
		};
		ofs += tmp;
		if (lseek(mailboxfd, curofs, SEEK_SET) < 0) {
			unlock_mailbox();
			pop_error("mailbox: lseek");
			send_error("fatal error");
			exit(1);
		};
		if (write(mailboxfd, mb_buf, tmp) != tmp) {
			unlock_mailbox();
			pop_error("mailbox: write");
			send_error("write error");
			exit(1);
		};
		curofs += tmp;
	};
}

void mb_update(void) {
	unsigned int tmp;
	off_t newsize;
	struct stat stbuf;
	
	if (mailboxfd < 0)
		return;
	if (lock_mailbox() < 0) {
		pop_log(pop_priority, "mailbox: can't set lock on mailbox: %.1024s", maildrop_name);
		pop_error(MLNAME);
		send_error("can't set lock on mailbox");
		exit(1);
	};
	if (fstat(mailboxfd, &stbuf) < 0) {
		unlock_mailbox();
		pop_error("mailbox: fstat");
		send_error("fstat failed");
		exit(1);
	};
	if (((newsize = stbuf.st_size) != lastmailboxsize) ||
	    (stbuf.st_mtime != mailboxmtime)) {
		if (stbuf.st_size < mailboxsize) {
			unlock_mailbox();
			pop_log(pop_priority, "mailbox: mailbox content has been changed: %.1024s", maildrop_name);
			send_error("fatal error");
			exit(1);
		};
		if (mb_parse(1) < 0) {
			unlock_mailbox();
			pop_log(pop_priority, "mailbox: mailbox content has been changed: %.1024s", maildrop_name);
			send_error("fatal error");
			exit(1);
		};
		lastmailboxsize = stbuf.st_size;
		mailboxmtime = stbuf.st_mtime;
	};
	if (lseek(mailboxfd, 0, SEEK_SET) < 0) {
		unlock_mailbox();
		pop_error("mailbox: lseek");
		send_error("fatal error");
		exit(1);
	};
	for (tmp = 0; tmp < msgnr; tmp++) {
		if ((messages[tmp].deleted))
			continue;
		mb_copy(((struct mb_message *) (messages[tmp].md_specific))->from_where, ((struct mb_message *) (messages[tmp].md_specific))->from_size);
	};
	if (newsize > mailboxsize)
		mb_copy(mailboxsize, newsize - mailboxsize);
	if ((newsize = lseek(mailboxfd, 0, SEEK_CUR)) < 0) {
		unlock_mailbox();
		pop_error("mailbox: lseek");
		send_error("fatal error");
		exit(1);
	};
	if ((mailboxsize = lseek(mailboxfd, 0, SEEK_END)) < 0) {
		unlock_mailbox();
		pop_error("mailbox: lseek");
		send_error("fatal error");
		exit(1);
	};
	if (mailboxsize != newsize)
		if (ftruncate(mailboxfd, newsize) < 0) {
			unlock_mailbox();
			pop_error("mailbox: ftruncate");
			send_error("fatal error");
			exit(1);
		};
	if (unlock_mailbox() < 0) {
		pop_log(pop_priority, "mailbox: can't remove lock from mailbox: %.1024s", maildrop_name);
		pop_error(MLNAME);
		send_error("can't remove lock from mailbox");
		exit(1);
	};
	if (close(mailboxfd) < 0) {
		pop_log(pop_priority, "mailbox: can't close mailbox: %.1024s", maildrop_name);
		pop_error("mailbox: close");
		send_error("can't close mailbox");
		exit(1);
	};
}

void mb_top(unsigned int nr, unsigned int lcount) {
	if (nr > msgnr) {
		send_error("no such message");
		check_wccount();
		return;
	};
	if (messages[--nr].deleted) {
		send_error("message %u is marked as deleted", nr + 1);
		check_wccount();
		return;
	};
	mb_reparse();
	if (lseek(mailboxfd, ((struct mb_message *) (messages[nr].md_specific))->where, SEEK_SET) < 0) {
		unlock_mailbox();
		pop_error("mailbox: lseek");
		exit(1);
	};
	md_top(nr, lcount, mailboxfd, &mb_cleanup);
	mb_reparse();
	md_end_reply(&mb_cleanup);
}

char *mb_md5_uidl_message(unsigned int number, char *result) {
	memcpy(result, ((struct mb_message *)(messages[number].md_specific))->digest, 16);
	return result;
}
