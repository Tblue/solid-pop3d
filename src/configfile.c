static const char rcsid[] = "$Id: configfile.c,v 1.3 2000/04/28 16:58:55 jurekb Exp $";
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
#include "configfile.h"
#include "log.h"
#include "maildrop.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#ifdef SPIPV6
#include "spipv6.h"
#endif

#ifdef NONIPVIRTUALS
char *cf_vhostname;
#endif
char *cf_name;
int cf_line, cf_column, ocf_line, ocf_column, oocf_column;
#ifdef SPIPV6
union sp_sockaddr skaddress;
#else
struct sockaddr_in skaddress;
#endif
int cf_fd;
char tbuf[1024];
int mcount, mwhere;
int cf_special;

void cf_error(char *cf_message) {
	pop_log(pop_priority, "config file: line %d, column %d: %.200s", ocf_line, ocf_column, cf_message);
}

int cf_get_char(void) {
	int tmp;

	if (mcount == 0) {
		if ((mcount = read(cf_fd, tbuf, sizeof(tbuf))) < 0) {
			pop_log(pop_priority, "config file: can't read from configuration file: %.1024s", cf_name);
			pop_error("config file: read");
			return -1; /* error */
		};
		if (mcount == 0)
			return -2; /* end of file */
		mwhere = 0;
	};
	mcount--;
	if ((tmp = tbuf[mwhere++]) == '\n') {
		cf_line++;
		oocf_column = cf_column;
		cf_column = 1;
	} else
		cf_column++;
	return tmp;
}
void cf_put_char(char what) { /* no error, it's called after cf_get_char() */
	tbuf[--mwhere] = what;
	mcount++;
	if (what == '\n') {
		cf_line--;
		cf_column = oocf_column;
	} else
		cf_column--;
	
}

int cf_skip_comment(void) {
	int tmp, tmp2;
	
	tmp = 0;
	while ((tmp2 = cf_get_char()) >= 0) {
		if ((tmp == '*') && (tmp2 == '/'))
			return 0;
		tmp = tmp2;
	};
	if (tmp == -2)
		cf_error("missing \"*/\"");
	return -1;
}


int cf_get_token(struct cf_token *result) {
	int tmp, tmp2, tokenlength = 0, quoted, maxlength;
	
	ocf_line = cf_line;
	ocf_column = cf_column;
	do {
		if ((tmp = cf_get_char()) < 0)
			return ((tmp == -1) ? -1 : 0);
	} while ((tmp == ' ') || (tmp == '\t'));
	if (tmp == '\n') {
		result->res_type = EOL;
		return 1;
	};
	if (tmp == '/') {		
		if ((tmp2 = cf_get_char()) < 0)
			if (tmp2 == -1)
				return -1;
		if (tmp2 != -2) {
			if (tmp2 == '*') {
				if (cf_skip_comment() < 0)
					return -1;
				return -2; /* call again */
			} else
				cf_put_char(tmp2);
		};
	};
	quoted = ((tmp == '\"') ? 1 : 0);
	if (quoted)
		if ((tmp = cf_get_char()) < 0) {
			result->res_string[tokenlength] = 0;
			if (tmp == -2)
				cf_error("missing '\"'");
			return -1;
		};
	result->res_type = STRING;
	maxlength = (result->max_size - 1);
	
	while (((quoted) || ((tmp != ' ') && (tmp != '\t') && (tmp != '\n'))) && (tmp != '\"') &&
		((tokenlength == 0) || (quoted) || (!cf_special) || (tmp != '>'))) {
		if (tmp == '\\') {
			if ((tmp = cf_get_char()) < 0) {
				result->res_string[tokenlength] = 0;
				if (tmp == -2)
					cf_error("some character after '\\' expected");
				return -1;
			};
			if (tmp == '\n') {
				if ((tmp = cf_get_char()) < 0) {
					result->res_string[tokenlength] = 0;
					if (quoted) {
						if (tmp == -2)
							cf_error("missing '\"'");
						return -1;
					};
					return ((tmp == -1) ? -1 : 1);
				};
				continue;
			};
		};
		if (maxlength == 0) {
			cf_error("argument too long");
			return -1;
		};
		maxlength--;
		result->res_string[tokenlength++] = tmp;
		if ((tmp = cf_get_char()) < 0) {
			result->res_string[tokenlength] = 0;
			if (quoted) {
				if (tmp == -2)
					cf_error("missing '\"'");
				return -1;
			};
			return ((tmp == -1) ? -1 : 1);
		};
	};
	result->res_string[tokenlength] = 0;
	if ((tmp == '\n') || ((tmp == '\"') && (!quoted)) || (tmp == '>'))
		cf_put_char(tmp);
	return 1;
}

int get_token(struct cf_token *result) {
	int tmp;
	
	while ((tmp = cf_get_token(result)) == -2);
	
	return tmp;
}

int parse_options_block(int thishost) {
	int tmp, opind;
	long int num;
	char *stolptr;
	struct cf_token p_token;
	char linebuf[PATH_MAX + 1];
	char numsuffix;
		
	p_token.max_size = sizeof(linebuf) - 1;
	p_token.res_string = linebuf;
	linebuf[0] = linebuf[PATH_MAX] = 0;
	if ((tmp = get_token(&p_token)) < 0)
		return -1;
	if (tmp == 0) {
		cf_error("option name expected");
		return -1;
	};
	do {
		while (p_token.res_type == EOL) {
			if ((tmp = get_token(&p_token)) < 0)
				return -1;
			if (tmp == 0) {
				cf_error("option name expected");
				return -1;
			};
		};
		if (p_token.res_type != STRING) {
			cf_error("option name expected");
			return -1;
		};
		if ((strcasecmp(linebuf, "</Global>") == 0) ||
		    (strcasecmp(linebuf, "</VirtualHost>") == 0))
			    return 0;
		opind = 0;
		while (options_set[opind].name != NULL) {
			if (strcasecmp(options_set[opind].name, linebuf) == 0)
				break;
			opind++;
		};
		if (options_set[opind].name == NULL) {
			cf_error("unknown option name");
			return -1;
		};		
		if ((tmp = get_token(&p_token)) < 0)
			return -1;
		if ((tmp == 0) || (p_token.res_type != STRING)) {
			cf_error("argument expected");
			return -1;
		};
		switch (options_set[opind].op_type) {
			case OP_STRING:
				if (strlen(linebuf) >= options_set[opind].valuesize) {
					cf_error("argument is too long");
					return -1;
				};
				if (options_set[opind].check_value != NULL)
					if (options_set[opind].check_value(linebuf) < 0) {
						cf_error("wrong argument");
						return -1;
					};
				if (thishost)
					strcpy((char *)options_set[opind].value, linebuf);
				tmp = get_token(&p_token);
				break;
			case OP_BOOLEAN:
				if ((strcasecmp(linebuf, "yes") == 0) ||
				    (strcasecmp(linebuf, "true") == 0)) {
					if (thishost)
						*((int *)(options_set[opind].value)) = 1;
				} else {
					if ((strcasecmp(linebuf, "no") == 0) ||
					    (strcasecmp(linebuf, "false") == 0)) {
						if (thishost)
							*((int *)(options_set[opind].value)) = 0;
					} else {
						cf_error("\"yes\", \"no\", \"true\" or \"false\" expected");
						return -1;
					};
				};
				tmp = get_token(&p_token);
				break;
#ifdef EXPIRATION
			case OP_EXPIRE:
				if (strcasecmp(linebuf, "never") == 0) {
					if (thishost)
						((struct expiration *)(options_set[opind].value))->enabled = 0;
					tmp = get_token(&p_token);
					break;
				}; /* no break here !!! */
#endif
			case OP_PERIOD:
				num = strtol(linebuf, &stolptr, 10);
				if (((stolptr[0] != 0) && (stolptr[1] != 0)) ||
			            (num == LONG_MIN) || (num == LONG_MAX)
				    || (num < 0)) {
					cf_error("wrong argument");
					return -1;
				};
				tmp = get_token(&p_token);
				numsuffix = *stolptr;
				if (*stolptr == 0) {
					if (tmp < 0)
						return -1;
					if (tmp == 0) {
						cf_error("\"end of line\" expected");
						return -1;
					};
					if (p_token.res_type == STRING) {
						if (strlen(linebuf) != 1) {
							cf_error("suffix too long");
							return -1;
						};
						numsuffix = linebuf[0];
						tmp = get_token(&p_token);
					};
				};				
				switch (numsuffix) {
					case 0:
					case 's':
						break;
					case 'm':
						num *= 60;
						break;
					case 'h':
						num *= 3600;
						break;
					case 'd':
						num *= (24*3600);
						break;
					case 'w':
						num *= (7*24*3600);
						break;
					default:
						cf_error("unknown suffix");
						return -1;
				};
#ifdef EXPIRATION
				if (options_set[opind].op_type == OP_EXPIRE) {
					if (thishost) {
						((struct expiration *)(options_set[opind].value))->enabled = 1;
						((struct expiration *)(options_set[opind].value))->expperiod = num;
					};
				} else
#endif
				if (thishost)
					*((unsigned int *)(options_set[opind].value)) = num;
		};
		if (tmp < 0)
			return -1;
		if ((tmp == 0) || (p_token.res_type != EOL))  {
			cf_error("\"end of line\" expected");
			return -1;
		};
		if ((tmp = get_token(&p_token)) < 0)
			return -1;
		if (tmp == 0) {
			cf_error("option name expected");
			return -1;
		};
	} while(1);
}

#ifdef SPIPV6
int sp_inet_aton(char *cp, struct in6_addr *buf) {
	struct in_addr tmp;
	
	if (inet_aton(cp, &tmp) > 0) {
		memset(buf, 0, sizeof(struct in6_addr));
		memcpy(&buf->s6_addr[12], &tmp, sizeof(tmp));
		buf->s6_addr[10] = buf->s6_addr[11] = 0xff;
		return 1;
	};
	return (inet_pton(AF_INET6, cp, buf) == 0) ? 0 : 1;
}
#endif

int cf_parse(void) {
	struct cf_token p_token;
	char linebuf[PATH_MAX];
	int tmp, thishost;
#ifdef NONIPVIRTUALS
	int nonipvh;
#endif
#ifdef SPIPV6
	struct in6_addr ipaddr;
#else
	struct in_addr ipaddr;
#endif
	
	p_token.max_size = sizeof(linebuf);
	p_token.res_string = linebuf;
	linebuf[0] = 0;
	mcount = mwhere = cf_special = 0;
	while(1) {
		if ((tmp = get_token(&p_token)) <= 0)
			return tmp; /* configuration file can end here */
		if (p_token.res_type == EOL)
			continue;
		if ((p_token.res_type != STRING) || 
		    ((strncasecmp(linebuf, "<VirtualHost", 12) != 0) &&
		     (strncasecmp(linebuf, "<Global>", 8) != 0))) {
			cf_error("<VirtualHost ...> or <Global> expected");
			return -1;
		};
		if (strncasecmp(linebuf, "<Global>", 8) == 0) { /* global */
			if ((tmp = get_token(&p_token)) < 0)
				return -1;
			if ((tmp == 0) || (p_token.res_type != EOL)) {
				if (tmp == 0)
					cf_error("\"end of line\" character expected");
				return -1;
			};
			if (parse_options_block(1) < 0)
				return -1;
		} else { /* virtualhost */
			cf_special = 1;
			if ((tmp = get_token(&p_token)) < 0)
				return -1;
			if ((tmp == 0) || (p_token.res_type != STRING)) {
#ifdef NONIPVIRTUALS
				cf_error("an IP address or virtual domain name expected");
#else
				cf_error("an IP address expected");
#endif
				return -1;
			};
			
#ifdef NONIPVIRTUALS
				nonipvh = 0;
#endif
#ifdef SPIPV6
			if (sp_inet_aton(linebuf, &ipaddr) == 0) {
#else
			if (inet_aton(linebuf, &ipaddr) == 0) {
#endif
#ifndef NONIPVIRTUALS
				cf_error("an IP address expected");
				return -1;
#else
				nonipvh = 1;
				if ((allownonip) && (cf_vhostname != NULL))
					thishost = (strcasecmp(cf_vhostname, linebuf) == 0) ? 1 : 0;
				else
					thishost = 0;
				if ((!allownonip) && (cf_vhostname != NULL))
					pop_log(pop_priority, "unallowed non-IP based virtual hosting request rejected");
#endif
			} else
#ifdef SPIPV6
				thishost = (memcmp(&skaddress.saddr_in6.sin6_addr, &ipaddr, sizeof(ipaddr)) == 0) ? 1 : 0;
#else
				thishost = (memcmp(&skaddress.sin_addr, &ipaddr, sizeof(ipaddr)) == 0) ? 1 : 0;
#endif
			
			if ((tmp = get_token(&p_token)) < 0)
				return -1;
			if (tmp == 0) {
				cf_error("not ended VirtualHost declaration");
				return -1;
			};
			if (p_token.res_type != STRING) {
#ifdef NONIPVIRTUALS
				if (!nonipvh)
					cf_error("a virtual domain name or '>' character expected");
				else
					cf_error("a '>' character expected");
				return -1;
#else
				cf_error("a '>' character expected");
#endif
			};
#ifdef NONIPVIRTUALS
			if (!nonipvh) {
				if (strcmp(linebuf, ">") != 0) {
					if (cf_vhostname != NULL)
						thishost |= ((strcasecmp(cf_vhostname, linebuf) == 0) ? 1 : 0);
					if ((tmp = get_token(&p_token)) < 0)
						return -1;
				};
				if ((tmp == 0) || (p_token.res_type != STRING)) {
					cf_error("a '>' character expected");
					return -1;
				};
			};
#endif
			cf_special = 0;
			if (strcmp(linebuf, ">") != 0) {
				cf_error("a '>' character expected");
				return -1;
			};
			if ((tmp = get_token(&p_token)) < 0)
				return -1;
			if ((tmp == 0) || (p_token.res_type != EOL)) {
				cf_error("\"end of line\" expected");
				return -1;
			};
			if (parse_options_block(thishost) < 0)
				return -1;
		};
	};
}

#ifdef NONIPVIRTUALS
int parse_config_file(char *name, char *vhostname) {
#else
int parse_config_file(char *name) {
#endif
#ifdef SPIPV6
	socklen_t addrln = sizeof(union sp_sockaddr);
#else
	socklen_t addrln = sizeof(struct sockaddr_in);
#endif
	int result;
	
#ifdef NONIPVIRTUALS
	cf_vhostname = vhostname;
#endif
	cf_name = name;
	if (getsockname(0, (struct sockaddr *) &skaddress, &addrln) < 0) {
		pop_error("config file: getsockname");
		return -1;
	};
#ifdef SPIPV6
	if ((skaddress.saddr_in.sin_family != AF_INET) &&
	    (skaddress.saddr_in6.sin6_family != AF_INET6)) {
#else
	if (skaddress.sin_family != AF_INET) {
#endif
		pop_log(pop_priority, "config file: socket address is not an IP address");
		return -1;
	};
#ifdef SPIPV6
	if (skaddress.saddr_in.sin_family == AF_INET) { /* change to IPv6 */
		struct in_addr tmpaddr;
		skaddress.saddr_in6.sin6_family = AF_INET6;
		tmpaddr = skaddress.saddr_in.sin_addr;
		memset(&skaddress.saddr_in6.sin6_addr, 0, sizeof(skaddress.saddr_in6.sin6_addr));
		memcpy(&skaddress.saddr_in6.sin6_addr.s6_addr[12], &tmpaddr, sizeof(tmpaddr));
		skaddress.saddr_in6.sin6_addr.s6_addr[10] = 0xff;
		skaddress.saddr_in6.sin6_addr.s6_addr[11] = 0xff;
	};
#endif
	if ((cf_fd = open(name, O_RDONLY)) < 0) {
		if (errno != ENOENT) {
			pop_log(pop_priority, "config file: can't open configuration file: %.1024s", name);
			pop_error("open");
			return -1;
		} else {
			pop_log_dbg(pop_priority, "config file: can't open configuration file: %.1024s", name);
			pop_error_dbg("open");
			return 0;
		};
	};
	cf_line = cf_column = 1;
	result = cf_parse();
	close(cf_fd);
	return result;
}

int check_maildrop_type(void *name) {
	return (find_maildrop((char*) name) != NULL) ? 0 : -1;
}
