static const char rcsid[] = "$Id: standalone.c,v 1.3 2000/04/28 16:58:55 jurekb Exp $";
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

/* The code is mostly ripped from popa3d. All bugs are probably introduced
by me. */

#include "includes.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
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
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include "log.h"
#ifdef SPIPV6
#include "spipv6.h"
#endif

extern int do_session(int, char **);
#if defined(DEBIAN) && defined(STANDALONE)
extern int standalone;
#endif
static volatile int blocked;
static volatile int pending;
int sckt;
struct session {
#ifdef SPIPV6
	struct in6_addr addr;
#else
	struct in_addr addr;
#endif
	pid_t pid;
	time_t start, log;
} sessions[MAX_SESSIONS];

void chld_handler(void) {
	pid_t spid;
	int tmp;
	
	if (blocked) {
		pending = 1;
		return;
	};
	
	while ((spid = waitpid(0, NULL, WNOHANG)) > 0)
		for (tmp = 0; tmp < MAX_SESSIONS; tmp++)
			if (sessions[tmp].pid == spid) {
				sessions[tmp].pid = 0;
				break;
			};
}

void sa_sig_handler(int num) {
	if (num == SIGCHLD) {
		chld_handler();
		signal(SIGCHLD, sa_sig_handler);
	};
}

int main(int argc, char **argv) {
#ifdef SPIPV6
	char ntopbuff[24];
	union sp_sockaddr address;
#else
	struct sockaddr_in address;
#endif
	struct rlimit core_limit = {0, 0};
	int session_socket;
	int tmp, persource, freeentry;
#ifdef SPIPV6
	socklen_t tmpaddrln = sizeof(struct sockaddr_in6);
#else
	socklen_t tmpaddrln = sizeof(struct sockaddr_in);
#endif
	time_t now;
	pid_t spid;
	
#if defined(DEBIAN) && defined(STANDALONE)
	/* Basic code pinched from Exim4's inetd detection */
#ifdef SPIPV6
	if (getpeername(0, (struct sockaddr *)&address.saddr_in6, &tmpaddrln) == 0) {
		int family = address.saddr_in6.sin6_family;;
#else
	if (getpeername(0, (struct sockaddr *)&address, &tmpaddrln) == 0) {
		int family = address.sin_family;;
#endif
		standalone = !(family == AF_INET || family == AF_INET6);
		if (!standalone) {
			do_session(argc,argv);
			/* do_session should never return */
			exit(1);
		}
	}
	else {
		standalone = 1;
	}

	for (tmp = getdtablesize(); tmp >= 0 ; --tmp)
		close(tmp);
	if (open("/dev/null", O_RDWR) == STDIN_FILENO) {
		dup2(STDIN_FILENO, STDOUT_FILENO);
		dup2(STDIN_FILENO, STDERR_FILENO);
	}
#endif
	pop_openlog();
	if (atexit(pop_closelog) < 0) {
		pop_error("atexit");
		pop_closelog();
		exit(1);
	};	
	if (setrlimit(RLIMIT_CORE, &core_limit) < 0) {
		pop_error("setrlimit");
		exit(1);
	};
	if (chdir("/") < 0) {
		pop_error("socket");
		exit(1);
	};
	signal(SIGCHLD, sa_sig_handler);

#ifdef SPIPV6
	address.saddr_in6.sin6_port = htons(POP3_PORT);
	address.saddr_in6.sin6_family = AF_INET6;
	address.saddr_in6.sin6_addr = in6addr_any;
#else	
	address.sin_port = htons(POP3_PORT);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
#endif

#ifdef SPIPV6
	if ((sckt = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		sckt = socket(AF_INET, SOCK_STREAM, 0);
		address.saddr_in.sin_port = htons(POP3_PORT);
		address.saddr_in.sin_family = AF_INET;
		address.saddr_in.sin_addr.s_addr = htonl(INADDR_ANY);
		if (sckt < 0) {
			pop_error("socket");
			exit(1);
		};
	};
#else
	if ((sckt = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		pop_error("socket");
		exit(1);
	};
#endif
	tmp = 1;
#ifdef HAVE_SETSOCKOPT
	if (setsockopt(sckt, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp)) < 0) {
		pop_error("setsockopt");
		exit(1);
	};
#endif
#ifdef SPIPV6
	if (bind(sckt, (struct sockaddr *)&address.saddr_in6, sizeof(union sp_sockaddr)) < 0) {
#else
	if (bind(sckt, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0) {
#endif
		pop_error("bind");
		exit(1);
	};
	if (listen(sckt, SOCKET_SYN_QUEUE) < 0) {
		pop_error("listen");
		exit(1);
	};
	
	switch (fork()) {
		case -1:
			pop_error("fork");
			close(sckt);
			exit(1);
		case 0:
			break;
		default:
			close(sckt);
			exit(0);
	};
	
	if (setsid() < 0) {
		pop_error("setsid");
		close(sckt);
		exit(0);
	};
	memset(sessions, 0, sizeof(struct session) * MAX_SESSIONS);	
	pending = session_socket = 0;	
	while (1) {
		blocked = 0;
		if (pending) {
			pending = 0;
			chld_handler();
		};
		if (session_socket > 0)
			if (close(session_socket) < 0) {
				pop_error("close");
				close(sckt);
				exit(1);
			};
#ifdef SPIPV6
		if ((session_socket = accept(sckt, (struct sockaddr*)&address.saddr_in6, &tmpaddrln)) < 0)
#else
		if ((session_socket = accept(sckt, (struct sockaddr*)&address, &tmpaddrln)) < 0)
#endif
			continue; /* Log it? Exit? */
		now = time(NULL);
		/* Check connection limits */
		persource = 0;
		freeentry = -1;
		blocked = 1;
#ifdef SPIPV6
		if (address.saddr_in6.sin6_family == AF_INET) { /* change to IPv6 */
			struct in_addr tmpaddr;
			address.saddr_in6.sin6_family = AF_INET6;
			tmpaddr = address.saddr_in.sin_addr;
			memset(&address.saddr_in6.sin6_addr, 0, sizeof(address.saddr_in6.sin6_addr));
			memcpy(&address.saddr_in6.sin6_addr.s6_addr[12], &tmpaddr, sizeof(tmpaddr));
			address.saddr_in6.sin6_addr.s6_addr[10] = 0xff;
			address.saddr_in6.sin6_addr.s6_addr[11] = 0xff;
		};
#endif
		for (tmp = 0; tmp < MAX_SESSIONS; tmp++) {
			if ((sessions[tmp].pid) ||
			    ((sessions[tmp].start) &&
			     ((now - sessions[tmp].start) < MIN_DELAY))) {
#ifdef SPIPV6
				if (memcmp(&sessions[tmp].addr, &address.saddr_in6.sin6_addr.s6_addr, sizeof(struct in6_addr)) == 0)
#else
				if (sessions[tmp].addr.s_addr == address.sin_addr.s_addr)
#endif
					if ((++persource) > PER_SOURCE)
						break;
			} else
				if (freeentry < 0)
					freeentry = tmp;
		};
		if (persource > PER_SOURCE) {
			if ((!sessions[tmp].log) ||
			    ((now - sessions[tmp].log) >= MIN_DELAY)) {
#ifdef SPIPV6
				pop_log(pop_priority, "per source limit exceeded: %.1024s", \
(strncmp(inet_ntop(AF_INET6, &address.saddr_in6.sin6_addr, ntopbuff, \
sizeof(ntopbuff)), "::ffff:", 7) == 0) ? ntopbuff + 7 : ntopbuff);
#else
				pop_log(pop_priority, "per source limit exceeded: %.1024s", inet_ntoa(address.sin_addr));
#endif
				sessions[tmp].log = now;
			};
			continue;
		};
		if (freeentry < 0) {
#ifdef SPIPV6
			pop_log(pop_priority, "session limit exceeded: %.1024s", \
(strncmp(inet_ntop(AF_INET6, &address.saddr_in6.sin6_addr, ntopbuff, \
sizeof(ntopbuff)), "::ffff:", 7) == 0) ? ntopbuff + 7 : ntopbuff);
			
#else
			pop_log(pop_priority, "session limit exceeded: %.1024s", inet_ntoa(address.sin_addr));
#endif
			continue;
		};
		switch ((spid = fork())) {
			case -1:
				close(session_socket);
				pop_error("fork");
				break;
			case 0:
#ifdef SPIPV6
				pop_log(pop_priority, "connect from %.1024s", \
(strncmp(inet_ntop(AF_INET6, &address.saddr_in6.sin6_addr, ntopbuff, \
sizeof(ntopbuff)), "::ffff:", 7) == 0) ? ntopbuff + 7 : ntopbuff);

#else
				pop_log(pop_priority, "connect from %.1024s", inet_ntoa(address.sin_addr));
#endif
				if (close(sckt) < 0) {
					close(session_socket);
					pop_error("close");
					exit(1);
				};
				pop_closelog();
				if (session_socket != 0)
					if (dup2(session_socket, 0) < 0) {
						close(session_socket);
						exit(1);
					};
				if (session_socket != 1)
					if (dup2(session_socket, 1) < 0) {
						close(0);
						close(session_socket);
						exit(1);
					};
				if (session_socket != 2)
					if (dup2(session_socket, 2) < 0) {
						close(0); close(1);
						close(session_socket);
						exit(1);
					};
				if (session_socket > 2)
					if (close(session_socket) < 0) {
						close(0); close(1); close(2);
						exit(1);
					};
				pop_openlog();
				do_session(argc, argv); /* fd: 0,1,2 - socket, 3 - syslog */
				pop_log(pop_priority, "Unexpected error - send info to author of program");
				exit(1); /* do_session() shouldn't return */
			default:
#ifdef SPIPV6
				sessions[freeentry].addr = address.saddr_in6.sin6_addr;
#else
				sessions[freeentry].addr = address.sin_addr;
#endif
				sessions[freeentry].pid = spid;
				sessions[freeentry].start = now;
				sessions[freeentry].log = 0;				
		};
	};
}
