static const char rcsid[] = "$Id: options.c,v 1.3 2000/04/28 16:58:55 jurekb Exp $";
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
#include "options.h"
#include "maildrop.h"
#ifdef MDMAILBOX
#include "mailbox.h"
#endif
#ifdef MDMAILDIR
#include "maildir.h"
#endif
#include "const.h"
#include "log.h"

extern int optopt, opterr;
extern char *optarg;

extern char maildrop_name[];
extern char maildrop_type[];
extern unsigned int autologout_time;
#ifdef ALLOWROOTLOGIN
extern int allow_root;
#endif
extern int pop_debug;

#ifdef NONIPVIRTUALS
extern int parse_config_file(char *, char *);
#else
extern int parse_config_file(char *);
#endif

#ifdef NONIPVIRTUALS
int parse_options(int argc, char **argv, char *vname) {
#else
int parse_options(int argc, char **argv) {
#endif
	int c;
	char *tmp2;
	long int tmp3;
#ifdef CONFIGFILE
	char configfile[PATH_MAX] = DEFCONFIGFILENAME;
#endif
	char tmpmaildrop_name[PATH_MAX], tmpmaildrop_type[MAXMDTYPENAMELENGTH];
	unsigned int tmpautologout_time = 0;
#ifdef ALLOWROOTLOGIN
	int tmpallow_root, used_allow_root = 0;
#endif
	int used_autologout_time = 0;
	
	opterr = 0;
	tmpmaildrop_name[0] = tmpmaildrop_type[0] = 0;
#ifdef CONFIGFILE
	while ((c = getopt(argc, argv, "n:t:a:r:f:d")) != EOF) {
#else
	while ((c = getopt(argc, argv, "n:t:a:r:d")) != EOF) {
#endif
		switch(c) {
			case 'n':
				if (strlen(optarg) >= PATH_MAX) {
					pop_log(pop_priority, "command line: maildrop name too long");
					return -1;
				};
				strcpy(tmpmaildrop_name, optarg);
				break;
			case 't':
				if (strlen(optarg) >= MAXMDTYPENAMELENGTH) {
					pop_log(pop_priority, "command line: maildrop type name too long");
					return -1;
				};
				if (find_maildrop(optarg) == NULL) {
					pop_log(pop_priority, "command line: no such maildrop type: %.40s", optarg);
					return -1;
				};
				strcpy(tmpmaildrop_type, optarg);
				break;
			case 'a':
				tmp3 = strtol(optarg, &tmp2, 0);
				if ((tmp3 == LONG_MIN) || (tmp3 == LONG_MAX)) {
					pop_log(pop_priority, "command line: autologout time value out of range");
					return -1;
				};
				if (tmp3 < 0) {
					pop_log(pop_priority, "command line: autologout time value out of range");
					return -1;
				};
				used_autologout_time = 1;
				switch (*tmp2) {
					case 0:
					case 's':
						tmpautologout_time = tmp3;
						used_autologout_time = 1;
						break;
					case 'm':
						tmpautologout_time = tmp3 * 60;
						used_autologout_time = 1;
						break;
					case 'h':
						tmpautologout_time = tmp3 * 3600;
						used_autologout_time = 1;
						break;
					case 'd':
						tmpautologout_time = tmp3 * 3600 * 24;
						used_autologout_time = 1;
						break;
					case 'w':
						tmpautologout_time = tmp3 * 3600 * 24 * 7;
						used_autologout_time = 1;
						break;
					default:
						pop_log(pop_priority, "command line: wrong suffix in \"-a\" option: %c", *tmp2);
						return -1;
				};
				break;
#ifdef ALLOWROOTLOGIN
			case 'r':
				tmpallow_root = used_allow_root = 1;
				break;
#endif
#ifdef CONFIGFILE
			case 'f':
				if ((strlen(optarg) + 1) >= sizeof(configfile)) {
					pop_log(pop_priority, "command line: config file name too long");
					return -1;
				};
				strcpy(configfile, optarg);
				break;
#endif
			case 'd':
				pop_debug = 1;
				break;
			case '?':
				pop_log(pop_priority, "command line: unknown option: %c", optopt);
				return -1;
			default:
				pop_log(pop_priority, "command line: unexpected error - contact author of program");
				return -1; 
		};
	};
#ifdef CONFIGFILE
#ifdef NONIPVIRTUALS
	if (parse_config_file(configfile, vname) < 0)
#else
	if (parse_config_file(configfile) < 0)
#endif
		return -1;
#endif
	if (tmpmaildrop_name[0] != 0)
		strcpy(maildrop_name, tmpmaildrop_name);
	if (tmpmaildrop_type[0] != 0)
		strcpy(maildrop_type, tmpmaildrop_type);
	if (used_autologout_time)
		autologout_time = tmpautologout_time;
#ifdef ALLOWROOTLOGIN
	if (used_allow_root)
		allow_root = allow_root;
#endif
	return 0;
}
