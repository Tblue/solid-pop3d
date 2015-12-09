static const char rcsid[] = "$Id: maildir.c,v 1.5 2000/05/13 13:25:52 jurekb Exp $";
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

#include "maildrop.h"
#include "maildir.h"
#include "const.h"
#include "md5.h"
#include "log.h"
#include "fdfgets.h"
#include "cmds.h"

#include <errno.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) (strlen((dirent)->d_name))
#else
#define dirent direct
#define NAMLEN(dirent) ((dirent)->d_namlen)
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
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
#include <stdint.h>

extern void check_wccount(void);

extern char maildrop_name[];
extern char username[];
extern int msgnr, maxmsgnr, msgdel;
extern struct message *messages;
int msgfd;
int mdir_exists;
#ifdef BULLETINS
pid_t nomsg = 0, nnomsg = 0;
#endif
#ifdef CREATEMAILDROP
extern int createmaildrop;
#endif

struct str_maildrop mdir_maildrop =
{
	mdir_init,
	mdir_update,
	mdir_release,
	mdir_retrieve,
	mdir_top,
#ifndef BULLETINS
	mdir_md5_uidl_message
#else
	mdir_md5_uidl_message,
	mdir_add_message,
	mdir_end_of_adding
#endif
};

int mdir_compare(const void *arg1, const void *arg2) {
        long int d1, d2;
    
	d1 = ((const struct message *)(arg1))->msg_time; 
        d2 = ((const struct message *)(arg2))->msg_time;
	if (d1 < d2)
	        return -1;
        if (d1 == d2)
		return 0;
	return 1;
}

#ifdef BULLETINS
void mdir_end_of_adding(void) {
	if (mdir_exists == 0)
		return;
	qsort(messages, msgnr, sizeof(struct message), mdir_compare);
}

int mdir_add_message(int fd) {
	char filename[PATH_MAX];
	char nfilename[PATH_MAX];
	char msgname[100 + 256];
	char hstname[256];
	pid_t tmppid = getpid() + nomsg;
	pid_t ntmppid = getpid() + nnomsg;
	time_t tmptime = time(NULL);
	int nmsgfd;
	ssize_t mcount = 0 , tmp = 0;
	char linebuf[128];
	
	if (mdir_exists == 0) {
		pop_log(pop_priority, "maildir: can't add bulletin - maildir \
doesn't exist");
		return 0;
	}
	if (gethostname(hstname, sizeof(hstname)) < 0) {
		close(fd);
		pop_log(pop_priority, "maildir: can't get name of host");
		pop_error("maildir: gethostname");
		send_error("fatal error");
		exit(1);
	};
	do {
		snprintf(msgname, sizeof(msgname), "tmp/%ju.%u.%.256s", (uintmax_t)tmptime, tmppid, hstname);
		if ((strlen(msgname) + strlen(maildrop_name) + 2) > sizeof(filename)) {
			close(fd);
			pop_log(pop_priority, "maildir: message file name is too long");
			send_error("fatal error");
			exit(1);
		};
		strcpy(filename, maildrop_name);
		strcat(filename, "/");
		strcat(filename, msgname);
		nmsgfd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0600);
		if ((nmsgfd < 0) && (errno == EEXIST)) {
			tmppid++;
			nomsg++;
			if (nomsg > 500) {
				close(fd);
				pop_log(pop_priority, "maildir: can't found unused message file name");
				send_error("fatal error");
				exit(1);
			};
			continue; /* try next message file name */
		};
		if (nmsgfd < 0) {
			close(fd);
			pop_log(pop_priority, "maildir: can't create message file: %.1024s", filename);
			pop_error("maildir: open");
			send_error("fatal error");
			exit(1);			
		};
		break;
	} while(1);
	fd_initfgets();
	if ((mcount = fd_fgets(linebuf, sizeof(linebuf), fd)) < 0) {
		close(nmsgfd);
		close(fd);
		unlink(filename);
		pop_log(pop_priority, "maildir: can't read message");
		pop_error("maildir: read");
		send_error("fatal error");
		exit(1);	
	};
	if (mcount > 5)
		if (strncmp(linebuf, "From ", 5) == 0) {
			while (linebuf[mcount - 1] != '\n') {
				if ((mcount = fd_fgets(linebuf, sizeof(linebuf), fd)) < 0) {
					close(nmsgfd);
					close(fd);
					unlink(filename);
					pop_log(pop_priority, "maildir: can't read message");
					pop_error("maildir: read");
					send_error("fatal error");
					exit(1);
				};
				if (mcount == 0)
					break;
			};
			if (mcount == 0) { /* null message */
				close(nmsgfd);
				close(fd);
				unlink(filename);
				pop_log(pop_priority, "maildir: empty message");
				send_error("fatal error");
				exit(1);
			};
			mcount = fd_fgets(linebuf, sizeof(linebuf), fd);
		};	
	if (md_realloc(sizeof(struct mdir_message)) < 0) {
		close(nmsgfd);
		close(fd);
		unlink(filename);
		pop_log(pop_priority, "maildir: no memory available");
		send_error("no memory available");
		exit(1);
	};
	msgnr++;
	
	messages[msgnr - 1].msg_time = tmptime;
	messages[msgnr - 1].deleted = messages[msgnr - 1].crlfsize =\
	messages[msgnr - 1].size = messages[msgnr - 1].cread = 0;
	while (mcount > 0) {
		if (write(nmsgfd, linebuf, mcount) != mcount) {
			close(nmsgfd);
			close(fd);
			unlink(filename);
			pop_log(pop_priority, "maildir: can't write to message file: %.1024s", filename);
			pop_error("maildir: write");
			send_error("fatal errror");
			exit(1);
		};
		tmp = mcount;
		messages[msgnr - 1].crlfsize += mcount;
		messages[msgnr - 1].size += mcount;
		if (linebuf[mcount - 1] == '\n')
			messages[msgnr - 1].crlfsize++;		
		mcount = fd_fgets(linebuf, sizeof(linebuf), fd);
	};
	if (mcount < 0) {
		close(nmsgfd);
		close(fd);
		unlink(filename);
		pop_log(pop_priority, "maildir: can't read from message file: %.1024s", filename);
		pop_error("maildir: read");
		send_error("can't read from message file");
		exit(1);
	};
	if (tmp > 0 && linebuf[tmp - 1] != '\n') {
		/* Message does not end with a newline; just add one to the file we are
		 * writing into the user's maildir. */
		if (write(nmsgfd, "\n", 1) != 1) {
			close(nmsgfd);
			close(fd);
			unlink(filename);
			pop_log(pop_priority, "maildir: can't write to message file: %.1024s", filename);
			pop_error("maildir: write");
			send_error("fatal errror");
			exit(1);
		}

		/* Pretend that the newline we just added was always present. */
		messages[msgnr - 1].size++;
		messages[msgnr - 1].crlfsize += 2;
	}
	if (close(nmsgfd) < 0) {
		close(fd);
		unlink(filename);
		pop_log(pop_priority, "maildir: can't close message file: %.1024s", filename);
		pop_error("maildir: close");
		send_error("can't close message file");
		exit(1);
	};
	do {
		snprintf(msgname, sizeof(msgname), "new/%ju.%u.%.256s", (uintmax_t)tmptime, ntmppid, hstname);
		if ((strlen(msgname) + strlen(maildrop_name) + 2) > sizeof(nfilename)) {
			close(fd);
			unlink(filename);
			pop_log(pop_priority, "maildir: message file name is too long");
			send_error("fatal error");
			exit(1);
		};
		strcpy(nfilename, maildrop_name);
		strcat(nfilename, "/");
		strcat(nfilename, msgname);
		nmsgfd = link(filename, nfilename);
		if ((nmsgfd < 0) && (errno == EEXIST)) {
			ntmppid++;
			nnomsg++;
			if (nnomsg > 500) {
				close(fd);
				unlink(filename);
				pop_log(pop_priority, "maildir: can't found unused message file name");
				send_error("fatal error");
				exit(1);
			};
			continue; /* try next message file name */
		};
		if (nmsgfd < 0) {
			close(fd);
			unlink(filename);
			pop_log(pop_priority, "maildir: can't link message file: %.1024s", nfilename);
			pop_error("maildir: link");
			send_error("fatal error");
			exit(1);			
		};
		break;
	} while(1);
	unlink(filename);
	if ((((struct mdir_message *) (messages[msgnr - 1].md_specific))->filename = (char *) malloc(strlen(msgname) + 1)) == NULL) {
		close(nmsgfd);
		close(fd);
		unlink(nfilename); /* remove message */
		pop_log(pop_priority, "maildir: no memory available");
		send_error("no memory available");
		exit(1);
	};
	strcpy(((struct mdir_message *) (messages[msgnr - 1].md_specific))->filename, msgname);
	return 1;
}
#endif

void mdir_clean(void) {
	struct stat stat_buf;
	DIR *dir;
	struct dirent *d_entry;
	char filename[PATH_MAX], *tmp;
	time_t act_time;
	
	if ((strlen(maildrop_name) + 7) > sizeof(filename))
		return;
	strcpy(filename, maildrop_name);
	strcat(filename, "/tmp/");
	tmp = filename + strlen(filename);
	act_time = time(NULL);
	if ((dir = opendir(filename)) == NULL)
		return;
	while ((d_entry = readdir(dir)) != NULL) {
		if (d_entry->d_name[0] == '.')
			continue;
		*tmp = 0;
		if ((strlen(filename) + NAMLEN(d_entry) + 1) > sizeof(filename))
			break;
		strcpy(tmp, d_entry->d_name);
		if (stat(filename, &stat_buf) < 0) {
			continue;
		};
		if (act_time > (stat_buf.st_atime + (36*3600)))
			unlink(filename);
	};
	closedir(dir);
	
}

void mdir_append(const char *directory) {
	DIR *dirstream;
	struct dirent *direntry;
	char filename[PATH_MAX];
	char mbuf[128];
	char *tmp;
	int messagefd;
	ssize_t tmp4, mcount;
	char *fname;
	struct stat stbuf;
	
	if ((strlen(directory) + strlen(maildrop_name) + 4) > sizeof(filename)) {
		mdir_release();
		pop_log(pop_priority, "maildir: maildir name too long");
		send_error("maildir name too long");
		exit(1);
	};
	strcpy(filename, maildrop_name);
	strcat(filename, "/");
	fname = filename + strlen(filename);
	strcat(filename, directory);	
	if ((dirstream = opendir(filename)) == NULL) {
#ifdef CREATEMAILDROP
		if ((createmaildrop) || (errno != ENOENT)) {
#else
		if (errno != ENOENT) {
#endif
			mdir_release();
			pop_log(pop_priority, "maildir: can't open maildir: %.1024s", filename);
			pop_error("maildir: opendir");
			send_error("can't open maildir");
			exit(1);
		}
		mdir_exists = 0;
		return;
	}
	strcat(filename, "/");
	tmp = filename + strlen(filename);
	while ((direntry = readdir(dirstream)) != NULL) {
		if (direntry->d_name[0] == '.')
			continue;
		if ((NAMLEN(direntry) + strlen(filename) + 1) > sizeof(filename)) {
			closedir(dirstream);
			mdir_release();
			pop_log(pop_priority, "maildir: message file name too long");
			send_error("message file name too long");
			exit(1);
		};
		strcpy(tmp, direntry->d_name);
		if (md_realloc(sizeof(struct mdir_message)) < 0) {
			closedir(dirstream);
			mdir_release();
			pop_log(pop_priority, "maildir: no memory available");
			send_error("no memory available");
			exit(1);
		};
		if ((((struct mdir_message *) (messages[msgnr].md_specific))->filename = (char *) malloc(strlen(fname) + 1)) == NULL) {
			closedir(dirstream);
			mdir_release();
			pop_log(pop_priority, "maildir: no memory available");
			send_error("no memory available");
			exit(1);
		};
		strcpy(((struct mdir_message *) (messages[msgnr].md_specific))->filename, fname);
		messages[msgnr].deleted = messages[msgnr].crlfsize =\
		messages[msgnr].size = messages[msgnr].cread = 0;
		if ((messagefd = open(filename, O_RDONLY)) < 0) {
			closedir(dirstream);
			mdir_release();
			pop_log(pop_priority, "maildir: can't open message file: %.1024s", filename);
			pop_error("maildir: open");
			send_error("can't open message file");
			exit(1);
		};
		if (fstat(messagefd, &stbuf) < 0) {
			closedir(dirstream);
			mdir_release();
			pop_log(pop_priority, "maildir: fstat failed: %.1024s", filename);
			pop_error("maildir: fstat");
			send_error("fatal error");
			exit(1);
		};
		messages[msgnr].msg_time = stbuf.st_mtime;
		msgnr++;
		*tmp = 0;
		fd_initfgets();
		tmp4 = 0;
		while ((mcount = fd_fgets(mbuf, sizeof(mbuf), messagefd)) > 0) {
			tmp4 = mcount;
			messages[msgnr - 1].crlfsize += mcount;
			messages[msgnr - 1].size += mcount;
			if (mbuf[mcount - 1] == '\n')
				messages[msgnr - 1].crlfsize++;
		};
		if (mcount < 0) {
			close(messagefd);
			closedir(dirstream);
			mdir_release();
			pop_log(pop_priority, "maildir: can't read from message file: %.1024s", filename);
			pop_error("maildir: read");
			send_error("can't read from message file");
			exit(1);
		};
		if (tmp4 > 0 && mbuf[tmp4 - 1] != '\n') {
			/* message has no trailing newline char. Pretend it had one so that we can
			 * output correct octet counts. When the message gets RETRieved, we actually
			 * add the missing newline char on-the-fly.
			 */
			messages[msgnr - 1].missing_eol = 1;
			messages[msgnr - 1].size++;
			messages[msgnr - 1].crlfsize += 2;
		}
		if (close(messagefd) < 0) {
			closedir(dirstream);
			mdir_release();
			pop_log(pop_priority, "maildir: can't close message file: %.1024s", filename);
			pop_error("maildir: close");
			send_error("can't close message file");
			exit(1);
		};
	};
	closedir(dirstream);
}

#ifdef CREATEMAILDROP
void mdir_mkdir(char *dirname) {
	if (mkdir(dirname, 0700) < 0) {
		pop_log(pop_priority, "maildir: can't create directory: %.1024s", dirname);
		pop_error("maildir: mkdir");
		send_error("fatal error");
		exit(1);
	};
}

void mdir_create(void) {
	char mdrop[PATH_MAX], *tmp;
	struct stat stbuf;
	
	if ((strlen(maildrop_name) + 5) > sizeof(mdrop)) {
		mdir_release();
		pop_log(pop_priority, "maildir: maildrop name too long");
		send_error("fatal error");
		exit(1);
	};
	strcpy(mdrop, maildrop_name);
	tmp = mdrop + strlen(mdrop);
	if (stat(mdrop, &stbuf) == 0)
		return;
	mdir_mkdir(mdrop);
	strcpy(tmp, "/new");
	mdir_mkdir(mdrop);
	strcpy(tmp, "/cur");
	mdir_mkdir(mdrop);
	strcpy(tmp, "/tmp");
	mdir_mkdir(mdrop);
}
#endif


void mdir_init()
{
	mdir_exists = 1;
	msgnr = 0;
	mdir_clean();	
	if (md_alloc(sizeof(struct mdir_message)) < 0) {		
		mdir_release();
		pop_log(pop_priority, "maildir: no memory available");
		send_error("no memory available");
		exit(1);
	};
#ifdef CREATEMAILDROP
	if (createmaildrop)
		mdir_create();
#endif
	mdir_append("new");
	if (mdir_exists)
		mdir_append("cur");
#ifndef BULLETINS
	if (mdir_exists)
		qsort(messages, msgnr, sizeof(struct message), mdir_compare);
#endif
}

void mdir_release()
{
	size_t number;

	if (messages == NULL)
		return;	
	for (number = 0; number < msgnr; number++)
		free(((struct mdir_message *) (messages[number].md_specific))->filename);
	md_free();
}

void mdir_cleanup(void) {
	close(msgfd);
}

void mdir_retrieve(unsigned int nr)
{
	char msgfile[PATH_MAX];

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
	if ((strlen(maildrop_name) + strlen(((struct mdir_message *)(messages[nr].md_specific))->filename)
	     + 2) > sizeof(msgfile)) { /* it was alredy checked in md_append, 
	                            	  but "szczezonego Pan Bog szczeze" */
		pop_log(pop_priority, "maildir: unexpected error");
		exit(1);
	};
	strcpy(msgfile, maildrop_name);
	strcat(msgfile, "/");
	strcat(msgfile, ((struct mdir_message *)(messages[nr].md_specific))->filename);
	if ((msgfd = open(msgfile, O_RDONLY)) < 0) {
		pop_log(pop_priority, "maildir: maildir content probably has been changed");
		pop_error("maildir: open");
		exit(1);
	};
	md_retrieve(nr, msgfd, &mdir_cleanup);
	md_end_reply(&mdir_cleanup);
	close(msgfd);
}

void mdir_update() {
	int tmp;
	char filename[PATH_MAX], newfilename[PATH_MAX], *tmp2;

	if (mdir_exists == 0)
		return;
	if ((strlen(maildrop_name) + 3) > sizeof(filename)) {
			    /* no overflow, already checked in mdir_append but
			       "szczezonego Pan Bog szczeze" */
		mdir_release();
		pop_log(pop_priority, "maildir: unexpected error - send info to author of program");
		send_error("fatal error");
		exit(1);
	};
	strcpy(filename, maildrop_name);
	strcat(filename, "/");
	tmp2 = filename + strlen(filename);
	for (tmp = 0; tmp < msgnr; tmp++) {
		*tmp2 = 0;
		if ((strlen(filename) + 
		     strlen(((struct mdir_message *) (messages[tmp].md_specific))->filename) +
		     1) > sizeof(filename)) {
			    /* no overflow, already checked in mdir_append but
			       "szczezonego Pan Bog szczeze" */
			mdir_release();
			pop_log(pop_priority, "maildir: unexpected error - send info to author of program");
			send_error("fatal error");
			exit(1);
		};
		strcpy(tmp2, ((struct mdir_message *) (messages[tmp].md_specific))->filename);
		if (messages[tmp].deleted)			
			unlink(filename);
		else {
			if (strncmp(tmp2, "new/", 4) != 0)
				continue;
			if ((strlen(filename) + 4) > sizeof(newfilename))
				continue;
			strcpy(newfilename, maildrop_name);
			strcat(newfilename, "/cur/");
			strcat(newfilename,((char *) (((struct mdir_message *) (messages[tmp].md_specific))->filename)) + 4);
			strcat(newfilename, ":2,"); /* no overflow, already checked few lines above */
			rename(filename, newfilename);
		};		
	};
	*tmp2 = 0;
}

void mdir_top(unsigned int nr, unsigned int lcount) {
	char msgfile[PATH_MAX];

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
	if ((strlen(maildrop_name) + strlen(((struct mdir_message *)(messages[nr].md_specific))->filename)
	     + 2) > sizeof(msgfile)) { /* it was alredy checked in md_append, 
	                            	  but "szczezonego Pan Bog szczeze" */
		pop_log(pop_priority, "maildir: unexpected error");
		exit(1);
	};
	strcpy(msgfile, maildrop_name);
	strcat(msgfile, "/");
	strcat(msgfile, ((struct mdir_message *)(messages[nr].md_specific))->filename);
	if ((msgfd = open(msgfile, O_RDONLY)) < 0) {
		pop_log(pop_priority, "maildir: maildir content probably has been changed");
		pop_error("maildir: open");
		exit(1);
	};
	md_top(nr, lcount, msgfd, &mdir_cleanup);
	md_end_reply(&mdir_cleanup);
	close(msgfd);
}

char *mdir_md5_uidl_message(unsigned int number, char *result) {
	size_t length;
	char *tmp2, *tmp3;
	struct md5_ctx context;
	
	md5_init_ctx(&context);
	tmp2 = ((struct mdir_message *) (messages[number].md_specific))->filename;
	if ((tmp3 = strrchr(tmp2,'/')) == NULL)
		tmp3 = tmp2;
	else
		tmp3 = tmp3 + 1;
	length = strlen(tmp3);
	if ((tmp2 = strchr(tmp3, ':')) != NULL)
		length -= strlen(tmp2);
	md5_process_bytes(tmp3, length, &context);
	md5_finish_ctx(&context, result);
	return result;
}

/* vim: set ts=4 sw=4 noet: */
