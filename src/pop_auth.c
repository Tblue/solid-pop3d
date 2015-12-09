static const char rcsid[] = "$Id: pop_auth.c,v 1.2 2000/04/28 16:58:55 jurekb Exp $";
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
#include <pwd.h>
#include <sys/file.h>
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
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <errno.h>
#include "const.h"
#include "maildrop.h"
#include "fdfgets.h"

int main(int argc, char **argv) {
	int fd, md_set = 0, linenr, fret;
	ssize_t tmp;
	struct passwd *pwentry;
	char username[9];
	char *pass;
	char passcopy[MAXARGLN + 1];
	char txtbuff[256], buf[128];
	struct rlimit corelimit = {0, 0};
	char cfgfile[PATH_MAX];
	struct stat stbuf;
	char *tmp2;
	char tmpmaildrop_type[MAXMDTYPENAMELENGTH], tmpmaildrop_name[PATH_MAX];
	static char digits[] = "0123456789abcdef";
	
	if (setrlimit(RLIMIT_CORE, &corelimit) < 0) {
		perror("setrlimit");
		return 1;
	};
	if ((pwentry = getpwuid(getuid())) == NULL) {
		fprintf(stderr, "can't find user with UID: %u\n", getuid());
		return 1;
	};
	username[0] = 0;
	strncat(username, pwentry->pw_name, 8);
	if (strlen(pwentry->pw_name) > 8)
		fprintf(stderr, "Warning: username truncated\n");
	snprintf(txtbuff, sizeof(txtbuff), "Enter NEW password for user %.40s: ", username);
	pass = getpass(txtbuff);
	passcopy[0] = 0;
	strncat(passcopy, pass, sizeof(passcopy) - 1);
	memset(pass, 0, strlen(pass));
	snprintf(txtbuff, sizeof(txtbuff), "Reenter NEW password for user %.40s: ", username);
	pass = getpass(txtbuff);
	fret = strcmp(pass, passcopy);
	memset(pass, 0, strlen(pass));
	if (fret != 0) {
		memset(passcopy, 0, strlen(passcopy));
		fprintf(stderr, "Passwords don't match\n");
		return 1;
	};
	if (stat(pwentry->pw_dir, &stbuf) < 0) {
		memset(passcopy, 0, strlen(passcopy));
		fprintf(stderr, "can't stat user home directory: %.1024s", pwentry->pw_dir);
		perror("stat");
		return 1;
	};			
	if (stbuf.st_mode & 2) {
		memset(passcopy, 0, strlen(passcopy));
		fprintf(stderr, "user home directory is world writable");
		return 1;
	};
	if ((strlen(pwentry->pw_dir) + strlen(USERCFG) + 2) > sizeof(cfgfile)) {
		memset(passcopy, 0, strlen(passcopy));
		fprintf(stderr, "home directory name too long");
		return 1;
	};
	strcpy(cfgfile, pwentry->pw_dir);
	strcat(cfgfile, "/");
	strcat(cfgfile, USERCFG);
	if ((fd = open(cfgfile, O_RDWR | O_CREAT, 0600)) < 0) {
		memset(passcopy, 0, strlen(passcopy));
		fprintf(stderr, "can't open user config file");
    		perror("open");
		return 1;
	};
	if (fstat(fd, &stbuf) < 0) {
		memset(passcopy, 0, strlen(passcopy));
		close(fd);
		fprintf(stderr, "can't stat user config file");
		perror("fstat");
		return 1;
	};
	if (!S_ISREG(stbuf.st_mode)) {
		memset(passcopy, 0, strlen(passcopy));
		close(fd);
		fprintf(stderr, "user config file is not regular file");
		return 1;
	};
	if (stbuf.st_mode & 0177) {
		memset(passcopy, 0, strlen(passcopy));
		close(fd);
		fprintf(stderr, "user config file has wrong mode");
		return 1;
	};
	fd_initfgets();
	linenr = 0;
	while (1) {
    		linenr++;
		if ((tmp = fd_fgets(buf, sizeof(buf), fd)) < 0) {
			memset(passcopy, 0, strlen(passcopy));
    			close(fd);
			fprintf(stderr, "can't read user config file");
			perror("read");
			return 1;
		};
		if (tmp == 0)
			break;
		if ((tmp == sizeof(buf)) && (buf[sizeof(buf) - 1] != '\n')) {
			memset(passcopy, 0, strlen(passcopy));
			close(fd);
			fprintf(stderr, "line too long in user config file");
			return 1;
		};
		if ((tmp == 1) && (buf[0] == '\n'))
			continue;
		if (buf[tmp - 1] == '\n')
			buf[tmp - 1] = 0;
		strtok(buf, " \t");
		if (strcasecmp(buf, "maildrop") == 0) {
			if (md_set == 1) {
				memset(passcopy, 0, strlen(passcopy));
				close(fd);
				fprintf(stderr, "maildrop already set in user config file");
				return 1;
			};
			if ((tmp2 = strtok(NULL, " \t")) == NULL) {
				memset(passcopy, 0, strlen(passcopy));
				close(fd);
				fprintf(stderr, "argument missing in maildrop declaration in user config file, line: %u", linenr);
				return 1;
			};
			if (strlen(tmp2) >= sizeof(tmpmaildrop_name)) {
				memset(passcopy, 0, strlen(passcopy));
				close(fd);
				fprintf(stderr, "maildrop file name in user config file is too long");
				return 1;
			};
			strcpy(tmpmaildrop_name, tmp2);
			if ((tmp2 = strtok(NULL, " \t")) == NULL) {
				memset(passcopy, 0, strlen(passcopy));
				close(fd);
				fprintf(stderr, "argument missing in maildrop declaration in user config file, line: %u", linenr);
				return 1;
			};
			if (strlen(tmp2) >= sizeof(tmpmaildrop_type)) {
				memset(passcopy, 0, strlen(passcopy));
				close(fd);
				fprintf(stderr, "maildrop type in user config file is too long");
				return 1;
			};
#ifdef MDMAILBOX
			if (strcasecmp(tmp2, "mailbox") != 0)
#endif
#ifdef MDMAILDIR
			if (strcasecmp(tmp2, "maildir") != 0)
#endif					    
			{
				memset(passcopy, 0, strlen(passcopy));
				close(fd);
				fprintf(stderr, "no such maildrop type: %.40s", tmp2);
    				return 1;
			};
			strcpy(tmpmaildrop_type, tmp2);
			md_set = 1;
			continue;
		};
		if (strcasecmp(buf, "APOPSecret") == 0)
			continue;
		memset(passcopy, 0, strlen(passcopy));
		close(fd);
		fprintf(stderr, "unknown option name, line: %u", linenr);
		return 1;
	};
	if (lseek(fd, 0, SEEK_SET) < 0) {
		memset(passcopy, 0, strlen(passcopy));
		close(fd);
		perror("lseek");
		return 1;
	};
	if (md_set == 1) {
		snprintf(txtbuff, sizeof(txtbuff), "MailDrop %.100s %.100s\n", tmpmaildrop_name, tmpmaildrop_type);
		if (write(fd, txtbuff, strlen(txtbuff)) < 0) {
			memset(passcopy, 0, strlen(passcopy));
			close(fd);
			perror("write");
			return 1;
		};
	};
	memset(txtbuff, 0, sizeof(txtbuff));
	strcpy(txtbuff, "APOPSecret ");
	tmp2 = txtbuff + strlen(txtbuff);
	for (tmp = 0; tmp < strlen(passcopy); tmp++) {
		passcopy[tmp] ^= 0xff;
		tmp2[tmp * 2] = digits[(passcopy[tmp] >> 4) & 0x0f];
		tmp2[(tmp * 2) + 1] = digits[passcopy[tmp] & 0x0f];				
	};
	memset(passcopy, 0, strlen(passcopy));
	strcat(txtbuff, "\n");
	if (write(fd, txtbuff, strlen(txtbuff)) < 0) {
		memset(txtbuff, 0, strlen(txtbuff));
		close(fd);
		perror("write");
		return 1;
	};
	memset(txtbuff, 0, strlen(txtbuff));
	if ((tmp = lseek(fd, 0, SEEK_CUR)) < 0) {
		close(fd);
		perror("lseek");
		return 1;
	};
	if (ftruncate(fd, tmp) < 0) {
		close(fd);
		perror("ftruncate");
		return 1;
	};
	if (close(fd) < 0) {
		perror("close");
		return 1;
	};
	return 0;
}
