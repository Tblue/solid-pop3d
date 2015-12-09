/* $Id: acconfig.h,v 1.2 2000/04/21 16:37:02 jurekb Exp $ */

/* Define if you want to use PAM to authenticate user.  */
#undef HAVE_PAM

/* Define if you have shadow passwords in /etc/shadow (Solaris style).  */
#undef HAVE_ETC_SHADOW

/* Define if you have shadow passwords in /etc/security/passwd.adjunct 
   (SunOS style). */
#undef HAVE_ETC_SECURITY_PASSWD_ADJUNCT

/* Define this if compiling on Ultrix. Defining those does not actually require
   shadow passwords to be present; this just includes support for them. */
#undef HAVE_ULTRIX_SHADOW_PASSWORDS

/* Define this if on SCO Unix */
#undef HAVE_SCO_ETC_SHADOW

/* Define this for HP-UX 10.x shadow passwords */
#undef HAVE_HPUX_TCB_AUTH

/* Define if you want to use APOP */
#undef APOP

/* Define if you want to have mailbox support */
#undef MDMAILBOX

/* Define if you want to have maildir support */
#undef MDMAILDIR

/* Define if you want to have configuration file support */
#undef CONFIGFILE

/* Define if you want to have user configuration support */
#undef USERCONFIG

/* Define if you want to have bulletins support */
#undef BULLETINS

/* Define if you want to have expiration support */
#undef EXPIRATION

/* Define if you want to compile standalone server */
#undef STANDALONE

/* Define if you want to have LAST command support */
#undef LASTCMD

/* Define to `int' if <sys/types.h> doesn't define */
#undef ssize_t

/* Define to `int' if <sys/types.h> doesn't define */
#undef socklen_t

/* Define if you have the maillock() function */
#undef MAILOCK

/* Define if you want to have support for user names mapping */
#undef MAPPING

/* Define if you want to have support for non-IP virtual hosting */
#undef NONIPVIRTUALS

/* Define if you want to have support for "AllowRootLogin" option */
#undef ALLOWROOTLOGIN

/* Define if you want to have support for "CreateMailDrop" option */
#undef CREATEMAILDROP

/* Define if you want to have support for IPv6 protocol */
#undef SPIPV6

/* Define if you want to have resolved peer name in logs */
#undef RESOLVE_HOSTNAME

/* If defined server logs "connect" message */
#undef LOG_CONNECT

/* Define if you want to have extended logging */
#undef LOG_EXTEND

/* Define if you want to have support for statistics */
#undef STATISTICS
