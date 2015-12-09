static const char rcsid[] = "$Id: main.c,v 1.8 2000/05/13 13:25:52 jurekb Exp $";
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
#include <sys/resource.h>
#include <pwd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#ifdef HAVE_GRP_H
#include <grp.h>
#endif
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
#ifdef BULLETINS
#ifdef HAVE_UTIME
#include <utime.h>
#endif
#endif
#if defined(LOG_EXTEND) || defined(LOG_CONNECT)
#ifdef RESOLVE_HOSTNAME
#include <netdb.h>
#endif
#endif
#include <ctype.h>

#include "response.h"
#include "const.h"
#include "cmds.h"
#include "maildrop.h"
#ifdef MDMAILBOX
#include "mailbox.h"
#endif
#ifdef MDMAILDIR
#include "maildir.h"
#endif
#include "options.h"
#include "log.h"
#include "userconfig.h"
#ifdef APOP
#include "apop.h"
#endif
#ifdef MAPPING
#include "mapping.h"
#endif
#ifdef SPIPV6
#include "spipv6.h"
#endif
#include "configfile.h"
#include "authenticate.h"

void get_username(char *);
void end_auth_state(char *);
void do_authentication(char *);
void do_stat(char *);
void do_list(char *);
void do_retrieve(char *);
void do_delete(char *);
void do_reset(char *);
void last_hack(char *);
void do_top(char *);
void do_uidl(char *);
#ifdef APOP
void do_apop(char *);
#endif
void end_trans_state(char *);

const struct s_cmd auth_cmds[] =
{
	{"USER", get_username},
	{"PASS", do_authentication},
	{"AUTH", not_implemented},
#ifdef APOP
	{"APOP", do_apop},
#endif
	{"QUIT", end_auth_state},
	{NULL, NULL}};

const struct s_cmd transaction_cmds[] =
{
	{"STAT", do_stat},
	{"LIST", do_list},
	{"RETR", do_retrieve},
	{"DELE", do_delete},
	{"NOOP", ignore_cmd},
#ifdef LASTCMD
	{"LAST", last_hack},
#endif
	{"RSET", do_reset},
	{"TOP", do_top},
	{"UIDL", do_uidl},
	{"QUIT", end_trans_state},
	{NULL, NULL}};

const struct s_cmd *cmds[] =
{auth_cmds, transaction_cmds};

struct str_option options_set[] =
{
#ifdef ALLOWROOTLOGIN
	{"AllowRootLogin", OP_BOOLEAN, &allow_root, 0, NULL},
#endif
#ifdef USERCONFIG
	{"UserOverride", OP_BOOLEAN, &useroverride, 0, NULL},
#endif
#ifdef APOP
	{"AllowAPOP", OP_BOOLEAN, &allowapop, 0, NULL},	
#endif
	{"MailDropName", OP_STRING, &maildrop_name, PATH_MAX, NULL},
#ifdef CONFIGFILE	
	{"MailDropType", OP_STRING, &maildrop_type, MAXMDTYPENAMELENGTH, &check_maildrop_type},
#else
	{"MailDropType", OP_STRING, &maildrop_type, MAXMDTYPENAMELENGTH, NULL},
#endif	
#ifdef APOP
	{"APOPServerName", OP_STRING, &apopservername, 256, NULL},
#endif
	{"AutoLogoutTime", OP_PERIOD, &autologout_time, 0, NULL},
	{"ChangeGID", OP_BOOLEAN, &changegid, 0, NULL},
	{"WrongCommandsLimit", OP_PERIOD, &wccount, 0, NULL},
#ifdef BULLETINS
	{"UserBullFile", OP_STRING, &userbullfile, PATH_MAX, NULL},
	{"BulletinDirectory", OP_STRING, &bulldir, PATH_MAX, NULL},
	{"AddBulletins", OP_BOOLEAN, &addbulletins, 0, NULL},
#endif
#ifdef EXPIRATION
	{"ReadExpire", OP_EXPIRE, &rexp, 0, NULL},
	{"UnreadExpire", OP_EXPIRE, &unrexp, 0, NULL},
#endif
#ifdef MAPPING
	{"UserMapFile", OP_STRING, &sp_mapfile, PATH_MAX, NULL},
	{"DoMapping", OP_BOOLEAN, &domapping, 0, NULL},
	{"RequiredMapping", OP_BOOLEAN, &reqmapping, 0, NULL},
#endif
#ifdef NONIPVIRTUALS
	{"AllowNonIP", OP_BOOLEAN, &allownonip, 0, NULL},
#endif
#ifdef CREATEMAILDROP
	{"CreateMailDrop", OP_BOOLEAN, &createmaildrop, 0, NULL},
#endif
#ifdef STATISTICS
	{"LogStatistics", OP_BOOLEAN, &logstatistics, 0, NULL},
#endif
	{"LogPriority", OP_STRING, &logpriority, 64, check_logpriority},
	{NULL, 0, NULL, 0, NULL}
};

int connection_state;
char username[MAXARGLN + 1];
char password[MAXARGLN + 1];
char buf[MAXCMDLN + 1];
size_t count = 0;
struct str_maildrop *maildrop;
#ifdef BULLETINS
char userbullfile[PATH_MAX];
char bulldir[PATH_MAX];
int addbulletins = 1;
#endif
#ifdef EXPIRATION
struct expiration rexp = {0, 0}, unrexp ={0, 0};
#endif
char maildrop_name[PATH_MAX];
char maildrop_type[MAXMDTYPENAMELENGTH];
#ifdef MAPPING
char sp_mapfile[PATH_MAX];
int domapping = 0, reqmapping = 1;
char mapusername[MAXARGLN + 1];
#endif
#ifdef APOP
char apop_secret[MAXARGLN + 1];
char apoptimestamp[MAXRESPLN];
int allowapop = 1;
#endif
#ifdef APOP
char apopservername[256] = {0, };
#endif
unsigned int autologout_time = DEFAUTOLOGOUTTIME, wccount = DEFWCCOUNT;
#ifdef ALLOWROOTLOGIN
int allow_root = 0;
#endif
#ifdef USERCONFIG
int useroverride = 1;
#endif
#ifdef NONIPVIRTUALS
int allownonip = 1;
#endif
int auth_finished = 0;
int changegid = 1;
int tunnel[2];
extern int msgnr;
extern struct message *messages;
#ifdef NONIPVIRTUALS
char *tmpvname;
#endif
#ifdef CREATEMAILDROP
int createmaildrop = 0;
#endif
#if defined(LOG_EXTEND) || defined(LOG_CONNECT)
char ahname[384];
#endif
#ifdef STATISTICS
int logstatistics = 1;
#endif

void check_wccount(void) {
	if (wccount != 0)
		if ((--wccount) == 0) {
#ifdef LOG_EXTEND
			pop_log(pop_priority, "wrong commands limit exceeded - %.384s", ahname);
#else
			pop_log(pop_priority, "wrong commands limit exceeded");
#endif
			exit(1);
		};
}

int expand_dir(char *dir, char *homedir) {
	char filename[PATH_MAX], *tmp, *tmp2;
	int breakwhile = 0;
	
	filename[0] = 0;
	tmp = dir;
	while(1) {
		if ((tmp2 = strchr(tmp, '%')) == NULL) {
			if ((strlen(filename) + strlen(tmp) + 1) > sizeof(filename)) {
				tmp2 = filename;
				break;
			};
			strcat(filename, tmp);
			break;
		};
		switch (tmp2[1]) {
			case '%':
				*(tmp2+1) = 0;
				if ((strlen(filename) + strlen(tmp) + 1) > sizeof(filename)) {
					breakwhile = 1;
					break;
				};
				strcat(filename, tmp);
				tmp = tmp2 + 2;
				break;
			case 's':
				
				*tmp2 = 0;
				if ((strlen(filename) + strlen(tmp) + strlen(username) + 1) > sizeof(filename)) {
				    breakwhile = 1;
				    break;
				};
				strcat(filename, tmp);
				strcat(filename, username);
				tmp = tmp2 + 2;
				break;
#ifdef MAPPING
			case 'm':
				*tmp2 = 0;
				if ((strlen(filename) + strlen(tmp) + strlen(mapusername) + 1) > sizeof(filename)) {
				    breakwhile = 1;
				    break;
				};
				strcat(filename, tmp);
				strcat(filename, mapusername);
				tmp = tmp2 + 2;
				break;
#endif
			case 'd':
				if ((tmp2[2] < '1') || (tmp2[2] > '8')) {
					breakwhile = 1;
					break;
				};
				*tmp2 = 0;
				if ((strlen(filename) + strlen(tmp) + 2) > sizeof(filename)) {
					breakwhile = 1;
					break;
				};
				strcat(filename, tmp);
				filename[(breakwhile = strlen(filename))] = username[tmp2[2] - '1'];
				/* if username is too short, just ignore it - username is padded with NULs */
				filename[breakwhile + 1] = 0;
				breakwhile = 0;
				tmp = tmp2 + 3;
				break;
			default: ;
		};
		if (breakwhile)
			break;
	};
	if (tmp2 != NULL)
		return -1;
	dir[0] = 0;
	if (filename[0] != '/') {
		if ((strlen(homedir) + 2) > PATH_MAX)
			return -1;
		strcpy(dir, homedir);
		strcat(dir, "/");
	};
	if ((strlen(dir) + strlen(filename) + 1) > PATH_MAX)
		return -1;
	strcat(dir, filename);
	return 0;
}

#ifdef BULLETINS
void add_bulletins(char *homedir) {
	char bullmsg[PATH_MAX];
	struct stat stbuf;
	int fd;
	DIR *dirstream;
	struct dirent *dentry;
	char *tmp;
	time_t modtime, actime;
#ifdef HAVE_UTIME
	struct utimbuf newmtime;
#else
	struct timeval newmtime[2];
#endif
	
	if (!addbulletins) {
		maildrop->md_end_of_adding();
		return;
	};
	if ((strlen(bulldir) + 1) >= sizeof(bullmsg)) {
		pop_log(pop_priority, "unexpected error - contact author of program");
		send_error("fatal error");
		exit(1);
	};
	strcpy(bullmsg, bulldir);
	strcat(bullmsg, "/");
	tmp = bullmsg + strlen(bullmsg);
	if (expand_dir(userbullfile, homedir) < 0) {
		pop_log(pop_priority, "file name too long");
		send_error("fatal error");
		exit(1);
	};
	modtime = 1;
	fd = open(userbullfile, O_RDONLY);
	if ((fd < 0) && (errno == ENOENT)) {
		fd = open(userbullfile, O_RDONLY | O_CREAT | O_EXCL, 0600);
		modtime = 0;
	};
	if (fd < 0) {
	/* It runs with user privileges, so users can do what they want */
		pop_log(pop_priority, "can't open or create file: %.1024s", userbullfile);
		pop_error("open");
		send_error("fatal error");
		exit(1);
	};
	if (fstat(fd, &stbuf) < 0) {
		close(fd);
		pop_log(pop_priority, "can't stat file: %.1024s", userbullfile);
		pop_error("stat");
		send_error("fatal error");
		exit(1);
	};
	if (modtime)
		modtime = stbuf.st_mtime;
	actime = stbuf.st_atime;
	close(fd); /* We have modification time of user bulletin file */
	if (!S_ISREG(stbuf.st_mode) || (stbuf.st_mode & 2) || (getuid() != stbuf.st_uid)) {
		pop_log(pop_priority, "user bulletin file has wrong mode");
		send_error("fatal error");
		exit(1);
	};
	if ((dirstream = opendir(bulldir)) == NULL) {
		pop_log(pop_priority, "opendir failed: %.1024s", bulldir);
		pop_error("opendir");
		send_error("fatal error");
		exit(1);
	};
	while ((dentry = readdir(dirstream)) != NULL) {
		if ((NAMLEN(dentry) + strlen(bullmsg) + 1) > sizeof(bullmsg)) {
			closedir(dirstream);
			pop_log(pop_priority, "file name too long");			
			send_error("fatal error");
			exit(1);
		};
		strcpy(tmp, dentry->d_name);
		if (stat(bullmsg, &stbuf) < 0) {
			closedir(dirstream);
			pop_log(pop_priority, "stat failed: %.1024s", bullmsg);
			pop_error("stat");
			send_error("fatal error");
			exit(1);
		};
		if (!S_ISREG(stbuf.st_mode))
			continue;
		if ((fd = open(bullmsg, O_RDONLY)) < 0) {
			closedir(dirstream);
			pop_log(pop_priority, "can't open file: %.1024s", bullmsg);
			pop_error("open");
			send_error("fatal error");
			exit(1);
		};
		if (modtime > stbuf.st_mtime) {
			close(fd);
			continue;
		};
		if (maildrop->md_add_message(fd) == 0) {
			close(fd);
			break;
		};
		close(fd);
	};
	closedir(dirstream);
	maildrop->md_end_of_adding();
#ifdef HAVE_UTIME
	newmtime.actime = actime;
	newmtime.modtime = time(NULL);
	if (utime(userbullfile, &newmtime) < 0) {
		pop_log(pop_priority, "can't set modification time of the file: %.1024s", userbullfile);
		pop_error("utime");
#else
	newmtime[0].tv_usec = newmtime[1].tv_usec = 0;
	newmtime[0].tv_sec = actime;
	newmtime[1].tv_sec = time(NULL);
	if (utimes(userbullfile, newmtime) < 0) {
		pop_error("utimes");
#endif		
		send_error("fatal error");
		exit(1);
	};
}
#endif


void at_transaction_end(void)
{
	maildrop->md_release();
}

void end_trans_state(char *arg)
{
#ifdef STATISTICS
	int deleted = 0, deletedsize = 0, left = 0, leftsize = 0;
#endif
#ifdef EXPIRATION
	int tmp;
	struct expiration *curexp;
	time_t acttime = time(NULL);

	for (tmp = 0; tmp < msgnr; tmp++) {
		if (messages[tmp].cread)
			curexp = &rexp;
		else
			curexp = &unrexp;
		if (curexp->enabled == 0)
			continue;
		if ((messages[tmp].msg_time == -1) ||
		    (messages[tmp].msg_time > acttime)) /* Clock skew??? */
			continue;
		if ((acttime - messages[tmp].msg_time) >= curexp->expperiod)
			messages[tmp].deleted = 1;
	};
	maildrop->md_update();
#endif /* EXPIRATION */

#ifdef STATISTICS
	for (tmp = 0; tmp < msgnr; tmp++)
		if (messages[tmp].deleted) {
			deleted++;
			deletedsize += messages[tmp].crlfsize;
		} else {
			left++;
			leftsize += messages[tmp].crlfsize;
		}
	if (logstatistics)
		pop_log(pop_priority, "Stats: %.40s %d %d %d %d", username, deleted, deletedsize, \
			left, leftsize);
#endif
#ifdef MAPPING
	if (domapping)
#ifdef LOG_EXTEND
		pop_log(pop_priority, "session ended for mapped user %.40s - %.384s", username, ahname);
#else
		pop_log(pop_priority, "session ended for mapped user %.40s", username);
#endif
	else
#ifdef LOG_EXTEND
		pop_log(pop_priority, "session ended for user %.40s - %.384s", username, ahname);
#else
		pop_log(pop_priority, "session ended for user %.40s", username);
#endif
#else /* MAPPING */
#ifdef LOG_EXTEND
	pop_log(pop_priority, "session ended for user %.40s - %.384s", username, ahname);
#else
	pop_log(pop_priority, "session ended for user %.40s", username);
#endif
#endif /* MAPPING */

	send_ok("session ended");
	exit(0);
}


void end_auth_state(char *arg) {
#ifdef LOG_EXTEND
	pop_log(pop_priority, "session ended - %.384s", ahname);
#else
	pop_log(pop_priority, "session ended");
#endif
	send_ok("session ended");
	exit(0);
}

void get_username(char *arg) {
	int tmp = 0;

	if (strlen(arg) >= sizeof(username)) {
		send_error("username too long");
		check_wccount();
	} else 
		if (arg[0] == 0) {
			send_error("missing argument");
			check_wccount();
		} else 
			if (username[0] == 0) {
				strncpy(username, arg, sizeof(username));
				/* Pad with NULs, strlen(arg) < sizeof(username) */
				while (username[tmp] != 0) {
					username[tmp] = tolower(username[tmp]);
					tmp++;
				};
				send_ok("username accepted");
			} else {
				send_error("username already set");
				check_wccount();
			};
}

#ifdef LASTCMD
void last_hack(char *arg) {
	send_ok("0");
}
#endif

void do_stat(char *arg)
{
	md_stat();
}

void do_list(char *arg)
{
	unsigned int number = 1;

	if (arg[0]) {
		if ((sscanf(arg, "%u", &number) != 1) || number == 0) {
			send_error("incorrect argument");
			check_wccount();
		} else
			md_list(number);
	} else
		md_list(0);
}

void do_retrieve(char *arg)
{
	unsigned int number = 1;

	if (!arg[0]) {
		send_error("argument required");
		check_wccount();
		return;
	};
	if ((sscanf(arg, "%u", &number) != 1) || number == 0) {
		send_error("incorrect argument");
		check_wccount();
	} else
		maildrop->md_retrieve(number);
}

void do_delete(char *arg)
{
	unsigned int number = 1;

	if (!arg[0]) {
		send_error("argument required");
		check_wccount();
		return;
	};
	if ((sscanf(arg, "%u", &number) != 1) || number == 0) {
		send_error("incorrect argument");
		check_wccount();
	} else
		md_delete(number);
}

void do_reset(char *arg)
{
	md_reset();
}

void do_top(char *arg)
{
	unsigned int number, lcount;
	if (!maildrop->md_top) {
/* TOP operation not supported by this maildrop */
		send_error("operation not supported by maildrop");
		check_wccount();
		return;
	};
	if ((sscanf(arg, "%u %u", &number, &lcount) != 2) || (number == 0)) {
		send_error("incorrect arguments");
		check_wccount();
		return;
	};
	maildrop->md_top(number, lcount);
}

void do_uidl(char *arg)
{
	unsigned int number = 1;

	if (!maildrop->md_md5_uidl_message) {
/* UIDL operation not supported by this maildrop */
		send_error("operation not supported by maildrop");
		check_wccount();
		return;
	};
	if (arg[0]) {
		if ((sscanf(arg, "%u", &number) != 1) || number == 0) {
			send_error("incorrect argument");
			check_wccount();
		}
		else
			md_uidl(number, maildrop->md_md5_uidl_message);
	} else
		md_uidl(0, maildrop->md_md5_uidl_message);
}


char *findcrlf(char *where, size_t length)
{				/*
				 * strstr stops on NUL character 
				 */
	size_t tmp;

	if (length == 0)
		return NULL;
	for (tmp = 0; tmp < (length - 1); tmp++)
		if ((where[tmp] == '\r') && (where[tmp + 1] == '\n'))
			return &where[tmp];
	return NULL;
}


void read_command(char *cmd)
{
	ssize_t cread;
	char *tmp;

	alarm(autologout_time);
	while (1) {
		while (((tmp = findcrlf(buf, count)) == NULL) && (count < MAXCMDLN)) {
			if ((cread = read(0, buf + count, sizeof(buf) - count - 1)) <= 0) {
				if (cread < 0)
					pop_error("read");
				exit(1);
			};
			count += cread;
		};
		if (tmp == NULL) {	/*
					 * command is too long 
					 */
			do {
				buf[0] = buf[count - 1];
				if ((cread = read(0, buf + 1, sizeof(buf) - 2)) <= 0) {
					if (cread < 0)
						pop_error("read");
					exit(1);
				};
				count = cread + 1;
			}
			while ((tmp = findcrlf(buf, count)) == NULL);
			memmove(buf, tmp + 2, count - (tmp - buf + 2));
			count -= (tmp - buf + 2);			
			send_error("command too long");
			check_wccount();
			continue;
		};
		*tmp = 0;
		if ((buf + strlen(buf)) != tmp) {
			send_error("NULL character in command");
			memmove(buf, tmp + 2, count - (tmp - buf + 2));
			count -= (tmp - buf + 2);	/*
							 * discard \r\n 
							 */
			check_wccount();
			continue;
		};
		strcpy(cmd, buf); /* strlen(buf) < sizeof(cmd)
		sizeof(buf) = sizeof(cmd) + 1 and \r\n in buf are discarded */
		memmove(buf, tmp + 2, count - (tmp - buf + 2));
		count -= (tmp - buf + 2);	/*
						 * discard \r\n 
						 */
		break;
	};
	alarm(0);
}

void sig_handler(int number) {
	if (number == SIGALRM)
#ifdef LOG_EXTEND
		pop_log(pop_priority, "autologout time elapsed - %.384s", ahname);
#else
		pop_log(pop_priority, "autologout time elapsed");
#endif
	exit(1);
}

ssize_t write_loop(int fd, void *abuf, size_t acount) {
	char *tmpbuf = (char *) abuf;
	size_t writed = 0;
	ssize_t tmp;
	
	if (acount == 0)
		return 0;
	while ((tmp = write(fd, tmpbuf + writed, acount - writed)) > 0) {
		writed += tmp;
		if (writed == acount)
			return writed;
	};
	if (tmp < 0)
		return -1;
	return writed;
}

int write_string(int fd, char *abuf) {
	size_t tmp = strlen(abuf);
	
	if (write_loop(fd, &tmp, sizeof(tmp)) != sizeof(tmp))
		return -1;
	if (write_loop(fd, abuf, tmp) != tmp)
		return -1;
	return 0;
}

void do_authentication(char *pass) {
#ifdef APOP
	int tmp = 0;
#endif
	
	if (username[0] == 0) {
		memset(pass, 0, strlen(pass));
		send_error("unknown username");
		check_wccount();
		return;
	};
#ifdef APOP
	if (write_loop(tunnel[1], &tmp, sizeof(tmp)) != sizeof(tmp)) {
		memset(pass, 0, strlen(pass));
		close(tunnel[1]);
		pop_log(pop_priority, "can't write to pipe");
		pop_error("write");
		send_error("fatal error");
		exit(1);
	};
#endif
#ifdef NONIPVIRTUALS
	tmpvname = strchr(username, '%');
	if (tmpvname == NULL)
		tmpvname = strchr(username, '@');
	if (tmpvname != NULL)
		*tmpvname = 0;
#endif
	if (write_string(tunnel[1], username) < 0) {
		memset(pass, 0, strlen(pass));
		close(tunnel[1]);
		pop_log(pop_priority, "can't write to pipe");
		pop_error("write");
		send_error("fatal error");
		exit(1);
	};
	if (write_string(tunnel[1], pass) < 0) {
		memset(pass, 0, strlen(pass));
		close(tunnel[1]);	
		pop_log(pop_priority, "can't write to pipe");
		pop_error("write");
		send_error("fatal error");
		exit(1);
	};
	memset(pass, 0, strlen(pass));
	auth_finished = 1;
	return;
}

#ifdef APOP
void do_apop(char *arg) {
	char *tmp;
	int tmp2;
	
	if (!allowapop) {
		memset(arg, 0, strlen(arg));
		send_error("you are not allowed to authenticate yourself through APOP");
		check_wccount();
		return;
	};
	if ((tmp = strchr(arg, ' ')) == NULL) {
		memset(arg, 0, strlen(arg));
		send_error("argument missing");
		check_wccount();
		return;
	};
	*tmp = 0;
	tmp++;
	if ((strlen(arg) + 1) > sizeof(username)) {
		memset(tmp, 0, strlen(tmp));
		send_error("username too long");
		check_wccount();
		return;
	};
	strncpy(username, arg, sizeof(username)); /* Pad with NULs */
	
	if (strlen(tmp) != 32) {
		memset(tmp, 0, strlen(tmp));
		send_error("wrong digest length");
		check_wccount();
		return;
	};
	for (tmp2 = 0; tmp2 < 16; tmp2++)
		if ((((tmp[tmp2 * 2] > 'f') || (tmp[tmp2 * 2] < 'a')) && (!isdigit(tmp[tmp2 * 2]))) ||
		    (((tmp[(tmp2 * 2) + 1] > 'f') || (tmp[(tmp2 * 2) + 1] < 'a')) && (!isdigit(tmp[(tmp2 * 2) + 1])))) {
		    	memset(tmp, 0, strlen(tmp));
			send_error("wrong character in digest");
			check_wccount();
			return;
		};
	tmp2 = 1;
	
	if (write_loop(tunnel[1], &tmp2, sizeof(tmp2)) != sizeof(tmp2)) {
		close(tunnel[1]);
		memset(tmp, 0, strlen(tmp));
		pop_log(pop_priority, "can't write to pipe");
		pop_error("write");
		send_error("fatal error");
		exit(1);
	};
	if (write_string(tunnel[1], apoptimestamp) < 0) {
		close(tunnel[1]);
		memset(tmp, 0, strlen(tmp));
		pop_log(pop_priority, "can't write to pipe");
		pop_error("write");
		send_error("fatal error");
		exit(1);
	};
#ifdef NONIPVIRTUALS
	tmpvname = strchr(username, '%');
	if (tmpvname == NULL)
		tmpvname = strchr(username, '@');
	if (tmpvname != NULL)
		*tmpvname = 0;
#endif
	if (write_string(tunnel[1], username) < 0) {
		close(tunnel[1]);
		memset(tmp, 0, strlen(tmp));
		pop_log(pop_priority, "can't write to pipe");
		pop_error("write");
		send_error("fatal error");
		exit(1);
	};
	if (write_string(tunnel[1], tmp) < 0) {
		close(tunnel[1]);
		memset(tmp, 0, strlen(tmp));
		pop_log(pop_priority, "can't write to pipe");
		pop_error("write");
		send_error("fatal error");
		exit(1);
	};
	memset(tmp, 0, strlen(tmp));
	auth_finished = 1;
	return;
}
#endif /* APOP */

ssize_t read_loop(int fd, void *abuf, size_t acount) {
/* This function works with root privileges */
	char *tmpbuf = (char *) abuf;
	ssize_t tmp, cread = 0;
	
	if (acount == 0)
		return 0;
	while ((tmp = read(fd, tmpbuf + cread, acount - cread)) > 0) {
		cread += tmp;
		if (cread == acount)
			return cread;
	};
	if (tmp < 0)
		return -1;
	return cread;
}

void child_corrupted(void) {
/* This function works with root privileges */
	memset(password, 0, sizeof(password));
	close(tunnel[0]);
	pop_log(pop_priority, "child process corrupted");
	wait(NULL);
	exit(1);
}

int read_string(int fd, char *abuf, size_t acount) {
/* This function works with root privileges */
	size_t tmp2;
	
	if (read_loop(fd, &tmp2, sizeof(tmp2)) != sizeof(tmp2))
		return -1;		
	if ((tmp2 >= acount) || (tmp2 < 0)) /* tmp2 is an unsigned number,
	so there wasn't security hole, however "szczezonego Pan Bog szczeze" */
		return -1;
	if (read_loop(fd, abuf, tmp2) != tmp2)
		return -1;
	abuf[tmp2] = 0;
	if (strlen(abuf) != tmp2)
		return -1;
	return 0;
}

int set_privileges(uid_t auid, gid_t agid) {
/* This function works with root privileges */
#ifdef HAVE_SETGROUPS
	gid_t groups[2];
	
	groups[0] = groups[1] = agid;
	if (setgroups(1, groups) < 0) {
		pop_error("setgroups");
		return -1;
	};
#endif
	if (setgid(agid) < 0) {
		pop_error("setgid");
		return -1;
	};
	if (setuid(auid) < 0) {
		pop_error("setuid");
		return -1;
	};
	return 0; /* I see no reason why this function could fail */
}

#ifdef STANDALONE
int do_session(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	char cmd[MAXCMDLN];
	const struct s_cmd *act_cmd;
#ifdef SPIPV6
	union sp_sockaddr peeraddr;
#else
	struct sockaddr_in peeraddr;
#endif
	socklen_t addrln;
	struct rlimit corelimit = {0, 0};
	struct passwd *spop3d, *userentry;
	int tmp, tmp2;
	gid_t tmpgid;
#ifdef APOP
	int useapop;
#endif
#if  defined(LOG_EXTEND) || defined(LOG_CONNECT)
#ifdef RESOLVE_HOSTNAME
	struct hostent *hentname;
#endif
#ifdef SPIPV6
	char ntopbuff[100];
#endif
#endif


	/* That below works as root */
	signal(SIGHUP, sig_handler);
	signal(SIGINT, sig_handler);
	signal(SIGQUIT, sig_handler);
	signal(SIGILL, sig_handler);
	signal(SIGABRT, sig_handler);
	signal(SIGFPE, sig_handler);
	signal(SIGSEGV, sig_handler);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGUSR1, sig_handler);
	signal(SIGUSR2, sig_handler);
	memset(username, 0, sizeof(username));
	memset(password, 0, sizeof(username));
#ifdef MAPPING
	memset(mapusername, 0, sizeof(mapusername));
#endif
	alarm(0);
	umask(0077);
	chdir("/");
	
#ifndef STANDALONE
	pop_openlog();
	if (atexit(pop_closelog) < 0)
		exit(1);
#endif
	if (setrlimit(RLIMIT_CORE, &corelimit) < 0) {
		pop_error("setrlimit");
		exit(1);
	};
#ifdef SPIPV6
	addrln = sizeof(union sp_sockaddr);
	if (getpeername(0, (struct sockaddr *)&peeraddr.saddr_in6, &addrln) < 0) {
#else
	addrln = sizeof(struct sockaddr_in);
	if (getpeername(0, (struct sockaddr *)&peeraddr, &addrln) < 0) {
#endif
		pop_error("getpeername");
		exit(1);
	};
#ifdef SPIPV6
	if ((peeraddr.saddr_in6.sin6_family != AF_INET) && (peeraddr.saddr_in6.sin6_family != AF_INET6)) {
#else
	if (peeraddr.sin_family != AF_INET) {
#endif
		pop_log(pop_priority, "peer address is not an IP address");
		exit(1);
	};
#if defined(LOG_EXTEND) || defined(LOG_CONNECT)
#ifdef RESOLVE_HOSTNAME
#ifdef SPIPV6
	if (peeraddr.saddr_in6.sin6_family == AF_INET6) {
		if ((hentname = gethostbyaddr((char *)&peeraddr.saddr_in6.sin6_addr, addrln, AF_INET6)) != NULL)
			snprintf(ahname, sizeof(ahname), "%.256s (%.100s)", hentname->h_name,
			    (strncmp(inet_ntop(AF_INET6, &peeraddr.saddr_in6.sin6_addr, ntopbuff, \
			    sizeof(ntopbuff)), "::ffff:", 7) == 0) ? ntopbuff + 7 : ntopbuff);
		else
			snprintf(ahname, sizeof(ahname), "%.100s",
			    (strncmp(inet_ntop(AF_INET6, &peeraddr.saddr_in6.sin6_addr, ntopbuff, \
			    sizeof(ntopbuff)), "::ffff:", 7) == 0) ? ntopbuff + 7 : ntopbuff);
	} else {
		if ((hentname = gethostbyaddr((char *)&peeraddr.saddr_in.sin_addr.s_addr, sizeof(peeraddr.saddr_in.sin_addr.s_addr), AF_INET)) != NULL)
			snprintf(ahname, sizeof(ahname), "%.256s (%.100s)", hentname->h_name,
			    inet_ntoa(peeraddr.saddr_in.sin_addr));
		else
			snprintf(ahname, sizeof(ahname), "%.100s",
			    inet_ntoa(peeraddr.saddr_in.sin_addr));

	};
#else
	if ((hentname = gethostbyaddr((char *)&peeraddr.sin_addr.s_addr, sizeof(peeraddr.sin_addr.s_addr), AF_INET)) != NULL)
		snprintf(ahname, sizeof(ahname), "%.256s (%.100s)", hentname->h_name,
		    inet_ntoa(peeraddr.sin_addr));
	else
		snprintf(ahname, sizeof(ahname), "%.100s",
		    inet_ntoa(peeraddr.sin_addr));
#endif
#else /* RESOLVE_HOSTNAME */
#ifdef SPIPV6
	if (peeraddr.saddr_in6.sin6_family == AF_INET6)
		snprintf(ahname, sizeof(ahname), "%.100s",
		    (strncmp(inet_ntop(AF_INET6, &peeraddr.saddr_in6.sin6_addr, ntopbuff, \
		    sizeof(ntopbuff)), "::ffff:", 7) == 0) ? ntopbuff + 7 : ntopbuff);
	else
		snprintf(ahname, sizeof(ahname), "%.100s",
		    inet_ntoa(peeraddr.saddr_in.sin_addr));
#else
	snprintf(ahname, sizeof(ahname), "%.100s",
	    inet_ntoa(peeraddr.sin_addr));
#endif
#endif /* RESOLVE_HOSTNAME */
#ifdef LOG_CONNECT
#ifndef STANDALONE
	pop_log(pop_priority, "connect from %.384s", ahname);
#endif
#endif
#endif /* LOG_EXTEND || LOG_CONNECT */

	if (pipe(tunnel) < 0) {
		pop_error("pipe");
		exit(1);
	};
	switch (fork()) {
		case -1:
			pop_error("fork");
			exit(1);
		case 0:
			close(tunnel[0]);
			if ((spop3d = getpwnam(POPUSER)) == NULL) {
				pop_log(pop_priority, "user %.40s not found", POPUSER);
				pop_error("getpwnam");
				exit(1);
			};
			if (set_privileges(spop3d->pw_uid, spop3d->pw_gid) < 0)
				exit(1);
/* Code below works as spop3d user */
			strcpy(maildrop_name, DEFMAILDROPNAME);
			strcpy(maildrop_type, DEFMAILDROPTYPE);
#ifdef BULLETINS
			strcpy(userbullfile, USERBULL);
			strcpy(bulldir, BULLDIR);
#endif


#ifdef NONIPVIRTUALS
			if (parse_options(argc, argv, NULL) < 0)
#else
			if (parse_options(argc, argv) < 0)
#endif
				exit(1);
#ifdef APOP
			if (allowapop) {
				if (apopservername[0] == 0)
					if (gethostname(apopservername, sizeof(apopservername)) < 0)
						exit(1);
				snprintf(apoptimestamp, sizeof(apoptimestamp), "<%lu.%lu@%.256s>", (unsigned long int) getpid(), (unsigned long int)time(NULL), apopservername);
				send_ok("%.100s %.400s", SERVER_GREETING, apoptimestamp);
			} else
				send_ok("%.100s", SERVER_GREETING);
#else
			send_ok(SERVER_GREETING);
#endif
			connection_state = AUTH_STATE;
			while (!auth_finished) {
				read_command(cmd);
				if ((act_cmd = cmd_lookup(cmds[connection_state], cmd)) == NULL) {
					send_error("unknown command");
					check_wccount();
					continue;
				};
				if (strlen(cmd) == strlen(act_cmd->name))
					act_cmd->handler(cmd + strlen(act_cmd->name));
				else
					act_cmd->handler(cmd + strlen(act_cmd->name) + 1);
			};
#ifdef NONIPVIRTUALS /* reread configuration file if needed */			
			if (tmpvname != NULL) {
				if (!allownonip)
					pop_log(pop_priority, "unallowed non-IP based virtual hosting request rejected");
				else
					if (parse_options(argc, argv, tmpvname + 1) < 0) {
						send_error("fatal error");
						exit(1);
					};
			};
#endif
#ifdef MAPPING
			mapusername[0] = 0;
			if (domapping) {
/* map_finduser() doesn't overflow mapusername - sizeof(mapusername) = MAXARGLN + 1 */
				if (((tmp = map_finduser(sp_mapfile, username, mapusername)) < 0) && \
				    (reqmapping)) {
#ifdef LOG_EXTEND
					pop_log(pop_priority, "authentication failed: can't map user name: %.40s - %.384s", username, ahname);
#else
					pop_log(pop_priority, "authentication failed: can't map user name: %.40s", username);
#endif
					sleep(1);
					send_error("authentication failed");
					exit(1);
				};
				if (tmp < 0) /* && !reqmapping */
					domapping = 0;
				else
					if ((strlen(mapusername) + 1) > sizeof(mapusername)) {
						pop_log(pop_priority, "map: unexpected error - contact author of program!!!");
						send_error("fatal error");
						exit(1);
					};
			};
			if (write_string(tunnel[1], mapusername) < 0) {
				close(tunnel[1]);
				pop_error("write");
				send_error("fatal error");
				exit(1);
			};
#endif
			tmp = 0;
			while (options_set[tmp].name != NULL) { /* This loop sends configuration */
				switch (options_set[tmp].op_type) {
					case OP_STRING:
						if (write_string(tunnel[1], (char *)options_set[tmp].value) < 0) {
							close(tunnel[1]);
							pop_error("write");
							send_error("fatal error");
							exit(1);
						};
						break;
					case OP_BOOLEAN:
						if (write_loop(tunnel[1], options_set[tmp].value, sizeof(int)) != sizeof(int)) {
							close(tunnel[1]);
							pop_error("write");
							send_error("fatal error");
							exit(1);
						};
						break;
					case OP_PERIOD:
						if (write_loop(tunnel[1], options_set[tmp].value, sizeof(unsigned int)) != sizeof(unsigned int)) {
							close(tunnel[1]);
							pop_error("write");
							send_error("fatal error");
							exit(1);
						};
						break;
#ifdef EXPIRATION
					case OP_EXPIRE:
						if (write_loop(tunnel[1], options_set[tmp].value, sizeof(struct expiration)) != sizeof(struct expiration)) {
							close(tunnel[1]);
							pop_error("write");
							send_error("fatal error");
							exit(1);
						};
#endif
				};
				tmp++;
			};
			close(tunnel[1]);
			exit(0);
		default: /* Code below reads configuration from child process.
			    Code below works with root privileges */
			close(tunnel[1]);
#ifdef APOP
			if (read_loop(tunnel[0], &useapop, sizeof(int)) != sizeof(int)) {
				close(tunnel[0]);
				wait(NULL);
				exit(1);
			};
			if ((useapop != 0) && (useapop != 1))
				child_corrupted();			
			if (useapop)
				if (read_string(tunnel[0], apoptimestamp, sizeof(apoptimestamp)) < 0)
					child_corrupted();
#endif
			
			if (read_string(tunnel[0], username, sizeof(username)) < 0) {
#ifdef APOP
				child_corrupted();
#else			
				close(tunnel[0]);
				wait(NULL);
				exit(1);
#endif
			};
			if (read_string(tunnel[0], password, sizeof(password)) < 0)
				child_corrupted();
#ifdef MAPPING
			memset(mapusername, 0, sizeof(mapusername));
			if (read_string(tunnel[0], mapusername, sizeof(mapusername)) < 0)
				child_corrupted();
#endif
			tmp = 0;
			while (options_set[tmp].name != NULL) {
				switch (options_set[tmp].op_type) {
					case OP_STRING:
						if (read_string(tunnel[0], (char *)options_set[tmp].value, options_set[tmp].valuesize) < 0)
							child_corrupted();
						break;
					case OP_BOOLEAN:
						if (read_loop(tunnel[0], options_set[tmp].value, sizeof(int)) != sizeof(int))
							child_corrupted();
						tmp2 = *((int *)(options_set[tmp].value));
						if ((tmp2 != 0) && (tmp2 != 1))
							child_corrupted();						
						break;
					case OP_PERIOD:
						if (read_loop(tunnel[0], options_set[tmp].value, sizeof(unsigned int)) != sizeof(unsigned int))
							child_corrupted();
						break;
#ifdef EXPIRATION
					case OP_EXPIRE:
						if (read_loop(tunnel[0], options_set[tmp].value, sizeof(struct expiration)) != sizeof(struct expiration))
							child_corrupted();
						break;
#endif
				};
				tmp++;
			};
			close(tunnel[0]);
			wait(NULL);
			check_logpriority(logpriority);
#ifdef MAPPING
			if (domapping)
				userentry = getpwnam(mapusername);
			else
				userentry = getpwnam(username);
#else
				userentry = getpwnam(username);
#endif
			if (userentry == NULL) {
				memset(password, 0, strlen(password));
#ifdef MAPPING
				if (domapping)
#ifdef LOG_EXTEND
					pop_log(pop_priority, "authentication failed: no such user: %.40s - %.384s", mapusername, ahname);
#else
					pop_log(pop_priority, "authentication failed: no such user: %.40s", mapusername);
#endif
				else
#ifdef LOG_EXTEND
					pop_log(pop_priority, "authentication failed: no such user: %.40s - %.384s", username, ahname);
#else
					pop_log(pop_priority, "authentication failed: no such user: %.40s", username);				
#endif
#else /* MAPPING */
#ifdef LOG_EXTEND
				pop_log(pop_priority, "authentication failed: no such user: %.40s - %.384s", username, ahname);
#else
				pop_log(pop_priority, "authentication failed: no such user: %.40s", username);				
#endif
#endif /* MAPPING */
				sleep(1);
				send_error("authentication failed");				
				exit(1);
			};			
#ifdef APOP
			apop_secret[0] = 0;
			if (!useapop) {
#endif
#ifdef MAPPING
			if (domapping)
				tmp = sp_authenticate_user(mapusername, password);
			else
				tmp = sp_authenticate_user(username, password);
#else
			tmp = sp_authenticate_user(username, password);
#endif	/* sp_authenticate_user clears password */
			if (tmp < 0) {
#ifdef MAPPING
				if (domapping)
#ifdef LOG_EXTEND
					pop_log(pop_priority, "authentication failed for mapped user %.40s - %.384s", username, ahname);
#else
					pop_log(pop_priority, "authentication failed for mapped user %.40s", username);
#endif
				else
#ifdef LOG_EXTEND
					pop_log(pop_priority, "authentication failed for user %.40s - %.384s", username, ahname);
#else
					pop_log(pop_priority, "authentication failed for user %.40s", username);
#endif
#else /* MAPPING */
#ifdef LOG_EXTEND
				pop_log(pop_priority, "authentication failed for user %.40s - %.384s", username, ahname);
#else
				pop_log(pop_priority, "authentication failed for user %.40s", username);
#endif
#endif /* MAPPING */
				sleep(1);
				send_error("authentication failed");
				exit(1);
			};
#ifdef MAPPING
			if (domapping)
#ifdef LOG_EXTEND
				pop_log(pop_priority, "mapped user %.40s authenticated - %.384s", username, ahname);
#else
				pop_log(pop_priority, "mapped user %.40s authenticated", username);
#endif
			else
#ifdef LOG_EXTEND
				pop_log(pop_priority, "user %.40s authenticated - %.384s", username, ahname);
#else
				pop_log(pop_priority, "user %.40s authenticated", username);
#endif
#else /* MAPPING */
#ifdef LOG_EXTEND
			pop_log(pop_priority, "user %.40s authenticated - %.384s", username, ahname);
#else
			pop_log(pop_priority, "user %.40s authenticated", username);
#endif

#endif /* MAPPING */
#ifdef APOP
			};			
#endif
#ifdef ALLOWROOTLOGIN
			if ((userentry->pw_uid == 0) && (!allow_root)) {
#else
			if (userentry->pw_uid == 0) {
#endif
				memset(password, 0, strlen(password));
#ifdef LOG_EXTEND
				pop_log(pop_priority, "root login not allowed - %.384s", ahname);
#else
				pop_log(pop_priority, "root login not allowed");
#endif
				sleep(1);
				send_error("authentication failed");
				exit(1);
			};
			if (changegid)
				tmpgid = userentry->pw_gid;
			else
				tmpgid = getgid();
			if (set_privileges(userentry->pw_uid, tmpgid) < 0) {
				memset(password, 0, strlen(password));
				send_error("fatal error");
				exit(1);
			};
/* Code below works with user privileges */
#ifdef USERCONFIG
			if (useroverride)
				parse_user_cfg(userentry->pw_dir);
#endif			
#ifdef APOP			
			if (useapop) {
				if (apop_secret[0] == 0) {
					memset(password, 0, strlen(password));
#ifdef LOG_EXTEND
					pop_log(pop_priority, "can't find APOP secret for user %.40s - %.384s", username, ahname);
#else
					pop_log(pop_priority, "can't find APOP secret for user %.40s", username);
#endif
					sleep(1);
					send_error("authentication failed");
					exit(1);
				};
				if (apop_authenticate(username, apoptimestamp, password) < 0) {
					memset(password, 0, strlen(password));
					memset(apop_secret, 0, strlen(apop_secret));
#ifdef LOG_EXTEND
					pop_log(pop_priority, "APOP authentication failed for user %.40s - %.384s", username, ahname);
#else
					pop_log(pop_priority, "APOP authentication failed for user %.40s", username);
#endif
					sleep(1);
					send_error("authentication failed");
					exit(1);
				};
#ifdef LOG_EXTEND
				pop_log(pop_priority, "user %.40s authenticated through APOP - %.384s", username, ahname);
#else
				pop_log(pop_priority, "user %.40s authenticated through APOP", username);
#endif
			};
			memset(apop_secret, 0, sizeof(apop_secret));
#endif
			memset(password, 0, strlen(password));
			if (expand_dir(maildrop_name, userentry->pw_dir) < 0) {
				pop_log(pop_priority, "maildrop name too long");
				sleep(1);
				send_error("authentication failed");
				exit(1);
			};
			maildrop = find_maildrop(maildrop_type);
			maildrop->md_init();
			if (atexit(at_transaction_end) < 0) {
				pop_log(pop_priority, "weird error");
				send_error("fatal error");
				exit(1);
			};
#ifdef BULLETINS
			add_bulletins(userentry->pw_dir);
#endif
			connection_state = TRANSACTION_STATE;
			send_ok("authentication successful");
			while (1) {
				read_command(cmd);
				if ((act_cmd = cmd_lookup(cmds[connection_state], cmd)) == NULL) {
					send_error("unknown command");
					check_wccount();
					continue;
				};
				if (strlen(cmd) == strlen(act_cmd->name))
					act_cmd->handler(cmd + strlen(act_cmd->name));
				else
					act_cmd->handler(cmd + strlen(act_cmd->name) + 1);
			};
	};
}
