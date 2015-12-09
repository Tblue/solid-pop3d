static const char rcsid[] = "$Id: authenticate.c,v 1.3 2000/04/28 16:58:55 jurekb Exp $";
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

/* Code from this file works with root privileges */

#include "includes.h"
#include <pwd.h>

#ifdef HAVE_PAM
#include <security/pam_appl.h>
#else /* HAVE_PAM */
#ifdef HAVE_ETC_SHADOW
#include <shadow.h>
extern char *crypt(const char *key, const char *salt);
#else /* HAVE_ETC_SHADOW */
#ifdef HAVE_SCO_ETC_SHADOW
#include <sys/security.h>
#include <sys/audit.h>
#include <prot.h>
#else /* HAVE_SCO_ETC_SHADOW */
#ifdef HAVE_HPUX_TCB_AUTH
#include <hpsecurity.h>
#include <prot.h>
#else /* HAVE_HPUX_TCB_AUTH */
#ifdef HAVE_ETC_SECURITY_PASSWD_ADJUNCT
#include <sys/label.h>
#include <sys/audit.h>
#include <pwdadj.h>
extern char *crypt(const char *key, const char *salt);
#else /* HAVE_ETC_SECURITY_PASSWD_ADJUNCT */
#ifdef HAVE_ULTRIX_SHADOW_PASSWORDS
#include <auth.h>
#include <sys/svcinfo.h>
#else /* HAVE_ULTRIX_SHADOW_PASSWORDS */
extern char *crypt(const char *key, const char *salt);
#endif /* HAVE_ULTRIX_SHADOW_PASSWORDS */
#endif /* HAVE_ETC_SECURITY_PASSWD_ADJUNCT */
#endif /* HAVE_HPUX_TCB_AUTH */
#endif /* HAVE_SCO_ETC_SHADOW */
#endif /* HAVE_ETC_SHADOW */
#endif /* HAVE_PAM */

#include "authenticate.h"
#include "const.h"
#include "log.h"
#include <sys/wait.h>

extern ssize_t write_loop(int, void *, size_t);

#ifdef HAVE_PAM
void freeresp(int numresp, struct pam_response *resp) {
	int respcount;
	
	for (respcount = 0; respcount < numresp; respcount++)
		if (resp[respcount].resp != NULL)
			free(resp[respcount].resp);
	free(resp);
}

int pconv(int num_msg, const struct pam_message **msg, struct pam_response **result, void *appdata_ptr) {
	int acount;
	struct pam_response *resp;
	
	resp = (struct pam_response *) calloc(num_msg, sizeof(struct pam_response));
	if (resp == NULL)
		return PAM_CONV_ERR;
	for (acount = 0; acount < num_msg; acount++)
		switch (msg[acount]->msg_style) {
			case PAM_PROMPT_ECHO_OFF:
				resp[acount].resp = strdup(((char **)appdata_ptr)[1]);
				if (resp[acount].resp == NULL) {
					freeresp(acount, resp);
					return PAM_CONV_ERR;
				};
				resp[acount].resp_retcode = PAM_SUCCESS;
				break;
			case PAM_PROMPT_ECHO_ON:
				resp[acount].resp = strdup(((char **)appdata_ptr)[0]);
				if (resp[acount].resp == NULL) {
					freeresp(acount, resp);
					return PAM_CONV_ERR;
				};
				resp[acount].resp_retcode = PAM_SUCCESS;
				break;
			case PAM_TEXT_INFO:
			case PAM_ERROR_MSG:
				resp[acount].resp_retcode = PAM_SUCCESS;
				resp[acount].resp = NULL;
				break;
			default:
				freeresp(acount, resp);
				return PAM_CONV_ERR;
		};
	*result = resp;
	return PAM_SUCCESS;
}

int _sp_authenticate_user(char *ausername, char *apassword) {
	pam_handle_t *pamhandle;
	char *app_data[2];
	struct pam_conv pamconv = {&pconv, &app_data};
	
	app_data[0] = ausername; app_data[1] = apassword;
	if (pam_start(SERVICE_NAME, ausername, &pamconv, &pamhandle) != PAM_SUCCESS)
		return -1;		
	if (pam_authenticate(pamhandle, 0) != PAM_SUCCESS) {
		pam_end(pamhandle, PAM_SUCCESS);
		return -1;
	};
	if (pam_acct_mgmt(pamhandle, 0) != PAM_SUCCESS) {
		pam_end(pamhandle, PAM_SUCCESS);
		return -1;
	};
	if (pam_setcred(pamhandle, PAM_ESTABLISH_CRED) != PAM_SUCCESS) {
		pam_end(pamhandle, PAM_SUCCESS);
		return -1;
	};
	pam_end(pamhandle, PAM_SUCCESS);
	return 0;
}
#else /* HAVE_PAM */
#if defined(_AIX) && defined(HAVE_AUTHENTICATE)
int _sp_authenticate_user(char *ausername, char *apassword) {
	char *message = NULL;
	int reenter = 1;
	
	if (authenticate(ausername, apassword, &reenter, &message) == 0)
		return 0;
	if (message != NULL)
		pop_log(pop_priority, "auth: %.200s", message);
	return -1;
};
#else /* defined(_AIX) && defined(HAVE_AUTHENTICATE) */
int _sp_authenticate_user(char *ausername, char *apassword) {
#ifdef HAVE_ETC_SHADOW
	struct spwd *sp;
#else /* HAVE_ETC_SHADOW */
#if defined(HAVE_SCO_ETC_SHADOW) || defined(HAVE_HPUX_TCB_AUTH)
	struct pr_passwd *pr;
#else /* defined(HAVE_SCO_ETC_SHADOW) || defined(HAVE_HPUX_TCB_AUTH) */
#ifdef HAVE_ETC_SECURITY_PASSWD_ADJUNCT
	struct passwd_adjunct *sp;
#else /* HAVE_ETC_SECURITY_PASSWD_ADJUNCT */
#ifdef HAVE_ULTRIX_SHADOW_PASSWORDS
	struct svcinfo *svp;
#endif /* HAVE_ULTRIX_SHADOW_PASSWORDS */
#endif /* HAVE_ETC_SECURITY_PASSWD_ADJUNCT */
#endif /* defined(HAVE_SCO_ETC_SHADOW) || defined(HAVE_HPUX_TCB_AUTH) */
#endif /* HAVE_ETC_SHADOW */
	char correct_passwd[128];
	char *encrypted_passwd;
	struct passwd *pwentry;
	
	if ((pwentry = getpwnam(ausername)) == NULL) {
		pop_log(pop_priority, "auth: getpwnam() failed");
		return -1;
	};	
	if (strcmp(pwentry->pw_passwd, "") == 0) {
		pop_log(pop_priority, "auth: null password entry in password file, user: %.40s", ausername);
		return -1;
	};
	correct_passwd[0] = 0;
	if (strlen(pwentry->pw_passwd) != 1)
		strncat(correct_passwd, pwentry->pw_passwd, sizeof(correct_passwd) - 1);
	else {
#ifdef HAVE_ETC_SHADOW
		if ((sp = getspnam(ausername)) == NULL) {
			endspent();
			pop_log(pop_priority, "auth: getspnam() failed");
			return -1;
		};
		correct_passwd[0] = 0;
		strncat(correct_passwd, sp->sp_pwdp, sizeof(correct_passwd) - 1);
		endspent();
#else /* HAVE_ETC_SHADOW */
#if defined(HAVE_SCO_ETC_SHADOW) || defined(HAVE_HPUX_TCB_AUTH)
		if ((pr = getprpwnam(ausername)) == NULL) {
			endprpwent();
			pop_log(pop_priority, "auth: getprpwnam() failed");
			return -1;
		};
		correct_passwd[0] = 0;
		strncat(correct_passwd, pr->ufld.fd_encrypt, sizeof(correct_passwd) - 1);
		endprpwent();
#else /* defined(HAVE_SCO_ETC_SHADOW) || defined(HAVE_HPUX_TCB_AUTH) */
#ifdef HAVE_ETC_SECURITY_PASSWD_ADJUNCT
		if ((sp = getpwanam(ausername)) == NULL) {
			endpwaent();
			pop_log(pop_priority, "auth: getpwanam() failed");
			return -1;
		};
		correct_passwd[0] = 0;
		strncat(correct_passwd, sp->pwa_passwd, sizeof(correct_passwd) - 1);
		endpwaent();
#else /* HAVE_ETC_SECURITY_PASSWD_ADJUNCT */
#ifdef HAVE_ULTRIX_SHADOW_PASSWORDS
		if ((svp = getsvc()) == NULL) {
			pop_log(pop_priority, "auth: getsvc() failed");
			return -1;
		};
		if (((svp->svcauth.seclevel == SEC_UPGRADE) &&
		     (strcmp(pwentry->pw_passwd, "*") == 0)) ||
		    (svp->svcauth.seclevel == SEC_ENHANCED))
			return ((authenticate_user(pwentry, apassword, "/dev/ttypXX") >= 0) ? 0 : -1);
		pop_log(pop_priority, "auth: can't authenticate user");
		return -1;
#else /* HAVE_ULTRIX_SHADOW_PASSWORDS */
		pop_log(pop_priority, "auth: no shadow password handling compiled in server");
		return -1;
#endif /* HAVE_ULTRIX_SHADOW_PASSWORDS */
#endif /* HAVE_ETC_SECURITY_PASSWD_ADJUNCT */
#endif /* defined(HAVE_SCO_ETC_SHADOW) || defined(HAVE_HPUX_TCB_AUTH) */
#endif /* HAVE_ETC_SHADOW */
	};
	if (strcmp(correct_passwd, "") == 0) {
			pop_log(pop_priority, "auth: null password entry in shadow file");
			return -1;
	};
#if defined(HAVE_SCO_ETC_SHADOW) || defined (HAVE_HPUX_TCB_AUTH)
	if (correct_passwd[0] && correct_passwd[1])
		encrypted_passwd = bigcrypt(apassword, correct_passwd);
	else
		encrypted_passwd = bigcrypt(apassword, "xx");
#else /* defined(HAVE_SCO_ETC_SHADOW) || defined (HAVE_HPUX_TCB_AUTH) */
	if (correct_passwd[0] && correct_passwd[1])
		encrypted_passwd = crypt(apassword, correct_passwd);
	else
		encrypted_passwd = crypt(apassword, "xx");
#endif /* defined(HAVE_SCO_ETC_SHADOW) || defined (HAVE_HPUX_TCB_AUTH) */
	return ((strcmp(encrypted_passwd, correct_passwd) == 0) ? 0 : -1);
}
#endif /* defined(_AIX) && defined(HAVE_AUTHENTICATE) */
#endif /* HAVE_PAM */

extern ssize_t read_loop(int, void *, size_t);

int sp_authenticate_user(char *ausername, char *apassword) {
#ifdef HAVE_PAM
	int tmp2;

	tmp2 = _sp_authenticate_user(ausername, apassword);
	memset(apassword, 0, strlen(apassword));
	return tmp2;
}
#else
	int tunnel[2], tmp2;

	if (pipe(tunnel) < 0) {	
		pop_error("auth: pipe");
		return -1;
	};
	switch (fork()) {
		case -1:
			pop_error("auth: fork");
			return -1;
		case 0: /* child process */
			close(tunnel[0]);
			tmp2 = (_sp_authenticate_user(ausername, apassword) == 0) ? 1 : 0;			
			memset(apassword, 0, strlen(apassword));
			if (write_loop(tunnel[1], &tmp2, sizeof(tmp2)) != sizeof(tmp2)) {
				pop_error("auth: write");
				close(tunnel[1]);
				exit(1);
			};
			exit(0);
		default: ;
	};
	close(tunnel[1]);
	memset(apassword, 0, strlen(apassword));
	if (read_loop(tunnel[0], &tmp2, sizeof(tmp2)) != sizeof(tmp2)) {
		close(tunnel[0]);
		wait(NULL);
		pop_error("auth: read");
		return -1;
	};
	close(tunnel[0]);
	wait(NULL);
	if ((tmp2 != 0) && (tmp2 != 1)) {
		pop_log(pop_priority, "auth: child process corupted");
		return -1;
	};
	return ((tmp2 == 1) ? 0 : -1);
}
#endif /* HAVE_PAM */
