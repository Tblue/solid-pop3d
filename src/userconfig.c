static const char rcsid[] = "$Id: userconfig.c,v 1.3 2000/04/28 16:58:55 jurekb Exp $";
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
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "maildrop.h"
#include "const.h"
#include "log.h"
#include "fdfgets.h"

extern char maildrop_name[];
extern char maildrop_type[];
#ifdef APOP
extern char apop_secret[];
#endif

void parse_user_cfg(char *homedir) {
	struct stat stbuf;
	char tmpmaildrop_name[PATH_MAX];
	char tmpmaildrop_type[MAXMDTYPENAMELENGTH];
#ifdef APOP
	char tmpapop_secret[MAXARGLN + 1];
#endif
	char cfgfile[PATH_MAX];
	char buf[128];
	int fd, md_set = 0;
#ifdef APOP
	int as_set = 0;
#endif
	int linenr;
	ssize_t tmp;
#ifdef APOP
	size_t tmp3;
#endif	
	char *tmp2;
		
	if (stat(homedir, &stbuf) < 0) {
		pop_log_dbg(pop_priority, "user config: can't stat user home directory: %.1024s", homedir);
		pop_error_dbg("user config: stat");
		return;
	};
	if (stbuf.st_mode & 2) {
		pop_log(pop_priority, "user config: user home directory is world writable");
		return;
	};
	if ((strlen(homedir) + strlen(USERCFG) + 2) > sizeof(cfgfile)) {
		pop_log(pop_priority, "user config: home directory name too long");
		return;
	};
	strcpy(cfgfile, homedir);
	strcat(cfgfile, "/");
	strcat(cfgfile, USERCFG);
	if ((fd = open(cfgfile, O_RDONLY)) < 0) {
		pop_log_dbg(pop_priority, "user config: can't open user config file");
		pop_error_dbg("user config: open");
		return;
	};
	if (fstat(fd, &stbuf) < 0) {
		close(fd);
		pop_log(pop_priority, "user config: can't stat user config file");
		pop_error("user config: fstat");
		return;
	};
	if (!S_ISREG(stbuf.st_mode)) {
		close(fd);
		pop_log(pop_priority, "user config: user config file is not regular file");
		return;
	};
	if (stbuf.st_mode & 0177) {
		close(fd);
		pop_log(pop_priority, "user config: user config file has wrong mode");
		return;
	};
	if (stbuf.st_size == 0) {
		close(fd);
		pop_log(pop_priority, "user config: user config file is empty");
		return;
	};
	fd_initfgets();
	linenr = 0;
	while (1) {
		linenr++;
		if ((tmp = fd_fgets(buf, sizeof(buf), fd)) < 0) {
			memset(buf, 0, sizeof(buf));
			close(fd);
			pop_log(pop_priority, "user config: can't read user config file");
			pop_error("user config: read");
			return;
		};
		if (tmp == 0)
			break;
		if ((tmp == sizeof(buf)) && (buf[sizeof(buf) - 1] != '\n')) {
			memset(buf, 0, sizeof(buf));
			close(fd);
			pop_log(pop_priority, "user config: line too long in user config file");
			return;
		};
		if ((tmp == 1) && (buf[0] == '\n'))
			continue;
		if (buf[tmp - 1] == '\n')
			buf[tmp - 1] = 0;
		strtok(buf, " \t");
		if (strcasecmp(buf, "maildrop") == 0) {
			if (md_set == 1) {
				memset(buf, 0, sizeof(buf));
				close(fd);
				pop_log(pop_priority, "user config: maildrop already set in user config file");
				return;
			};
			if ((tmp2 = strtok(NULL, " \t")) == NULL) {
				memset(buf, 0, sizeof(buf));
				close(fd);
				pop_log(pop_priority, "user config: argument missing in maildrop declaration in user config file, line: %u", linenr);
				return;
			};
			if (strlen(tmp2) >= sizeof(tmpmaildrop_name)) {
				memset(buf, 0, sizeof(buf));
				close(fd);
				pop_log(pop_priority, "user config: maildrop file name in user config file is too long");
				return;
			};
			strcpy(tmpmaildrop_name, tmp2);
			if ((tmp2 = strtok(NULL, " \t")) == NULL) {
				memset(buf, 0, sizeof(buf));
				close(fd);
				pop_log(pop_priority, "user config: argument missing in maildrop declaration in user config file, line: %u", linenr);
				return;
			};
			if (strlen(tmp2) >= sizeof(tmpmaildrop_type)) {
				memset(buf, 0, sizeof(buf));
				close(fd);
				pop_log(pop_priority, "user config: maildrop type in user config file is too long");
				return;
			};
#ifdef MDMAILBOX
			if (strcasecmp(tmp2, "mailbox") != 0)
#endif
#ifdef MDMAILDIR
			if (strcasecmp(tmp2, "maildir") != 0)
#endif					    
			{
				memset(buf, 0, sizeof(buf));
				close(fd);
				pop_log(pop_priority, "user config: no such maildrop type: %.40s", tmp2);
				return;
			};
			strcpy(tmpmaildrop_type, tmp2);
			md_set = 1;
			continue;
		};
#ifdef APOP
		if (strcasecmp(buf, "APOPsecret") == 0) {
			if (as_set == 1) {
				memset(buf, 0, sizeof(buf));
				close(fd);
				pop_log(pop_priority, "user config: APOP secret already set in user config file");
				return;
			};
			if ((tmp2 = strtok(NULL, " \t")) == NULL) {
				memset(buf, 0, sizeof(buf));
				close(fd);
				pop_log(pop_priority, "user config: argument missing, line: %u", linenr);
				return;
			};
			tmp3 = strlen(tmp2);
			if (tmp3 & 1) {
				memset(buf, 0, sizeof(buf));
				close(fd);
				pop_log(pop_priority, "user config: length of encrypted APOP secret should be even number");
				return;
			};
			if ((tmp3/2) >= sizeof(tmpapop_secret)) {
				memset(buf, 0, sizeof(buf));
				close(fd);
				pop_log(pop_priority, "user config: encrypted APOP secret is too long");				
				return;
			};
			for (tmp3 = 0; tmp3 < strlen(tmp2); tmp3++)
				if (((tmp2[tmp3] < 'a') || (tmp2[tmp3] > 'z')) &&
				    (!isdigit(tmp2[tmp3]))) {
					    memset(buf, 0, sizeof(buf));
					    close(fd);
					    pop_log(pop_priority, "user config: wrong character in encrypted APOP secret");					    
					    return;
				    };
			memset(tmpapop_secret, 0, sizeof(tmpapop_secret));
			for (tmp3 = 0; tmp3 < (strlen(tmp2) / 2); tmp3++) {
				if (isdigit(tmp2[tmp3 * 2]))
					tmpapop_secret[tmp3] = (tmp2[tmp3 * 2] - '0') << 4;
				else
					tmpapop_secret[tmp3] = (tmp2[tmp3 * 2] - 'a' + 10) << 4;
				if (isdigit(tmp2[(tmp3 * 2) + 1]))
					tmpapop_secret[tmp3] |= (tmp2[(tmp3 * 2) + 1] - '0');
				else
					tmpapop_secret[tmp3] |= (tmp2[(tmp3 * 2) + 1] - 'a' + 10);
				tmpapop_secret[tmp3] ^= 0xff;
			};
			as_set = 1;
			continue;
		};
		memset(tmpapop_secret, 0, sizeof(tmpapop_secret));
#endif
		memset(buf, 0, sizeof(buf));
		close(fd);
		pop_log(pop_priority, "user config: unknown option name, line: %u", linenr);
		return;
	};
	close(fd);
	if (md_set == 1) {
		strcpy(maildrop_name, tmpmaildrop_name);
		strcpy(maildrop_type, tmpmaildrop_type);
	};
#ifdef APOP
	if (as_set == 1)
		strcpy(apop_secret, tmpapop_secret);
	memset(tmpapop_secret, 0, sizeof(tmpapop_secret));
#endif
	return;
}
