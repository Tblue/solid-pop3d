# Process this file with autoconf to produce a configure script.
AC_INIT([solid-pop3d], [0.15])
AC_CONFIG_SRCDIR([src/authenticate.c])
AC_CONFIG_HEADERS([config.h])

# autoheader templates
# (only add a call to AH_TEMPLATE here if there are
# multiple AC_DEFINE calls for a variable)
AH_TEMPLATE([CONFIGFILE],
			[Define if you want to have configuration
			 file support])
AH_TEMPLATE([EXPIRATION],
			[Define if you want to have expiration
			 support])
AH_TEMPLATE([HAVE_ETC_SECURITY_PASSWD_ADJUNCT],
			[Define if you have shadow passwords in
			 /etc/security/passwd.adjunct (SunOS style).])
AH_TEMPLATE([HAVE_ETC_SHADOW],
			[Define if you have shadow passwords in
			 /etc/shadow (Solaris style).])
AH_TEMPLATE([HAVE_SCO_ETC_SHADOW],
			[Define this if on SCO Unix])
AH_TEMPLATE([MAILOCK],
			[Define if you have the maillock() function])
AH_TEMPLATE([MDMAILBOX],
			[Define if you want to have mailbox support])
AH_TEMPLATE([MDMAILDIR],
			[Define if you want to have maildir support])
AH_TEMPLATE([USERCONFIG],
			[Define if you want to have user configuration
			 support])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_CANONICAL_HOST

dnl Checks for authenticate methods (mostly ripped from SSH)
SPOPLIBS=""
case "$host" in
	*-*-sunos4.1.1*)
		os_sunos=yes
		;;
	*-*-sunos*)
		os_sunos=yes
		;;
	*-sgi-irix5*)
		AC_DEFINE(HAVE_ETC_SHADOW)
		no_shadow_password_checking=yes
		no_libsun=yes
		;;
	*-sgi-irix6*)
		AC_DEFINE(HAVE_ETC_SHADOW)
		no_shadow_password_checking=yes
		no_libsun=yes
		;;
	*-dec-ultrix*)
		AC_DEFINE([HAVE_ULTRIX_SHADOW_PASSWORDS], [],
				  [Define this if compiling on Ultrix.
				   Defining those does not actually
				   require shadow passwords to be present;
				   this just includes support for them.])
		AC_CHECK_LIB(auth, authenticate_user, [
			SPOPLIBS="$SPOPLIBS -lauth"] )
		no_shadow_password_checking=yes
		;;
	*-*-hpux*)
		AC_MSG_CHECKING(for HPUX tcb auth option)
		if test -f /tcb/files/auth/system/pw_id_map; then
			AC_MSG_RESULT(yes)
			AC_DEFINE([HAVE_HPUX_TCB_AUTH], [],
					  [Define this for HP-UX 10.x shadow
					   passwords])
			SPOPLIBS="$SPOPLIBS -lsec"
			no_shadow_password_checking=yes
		else
			AC_MSG_RESULT(no)
		fi
		;;
	*-*-linux*)
		AC_CHECK_FUNC(getspnam, [
			getspnam_found=yes])
		if test -z "$getspnam_found" ; then
			AC_CHECK_LIB(shadow, getspnam, [
				getspnam_found=yes
				SPOPLIBS="$SPOPLIBS -lshadow"] )
		fi
		no_shadow_password_checking=yes
		AC_DEFINE(HAVE_ETC_SHADOW)
		;;
	i*86-unknown-sco3.2v4*)
		AC_DEFINE(HAVE_SCO_ETC_SHADOW)
		no_shadow_password_checking=yes
		;;
	i*86-unknown-sco3.2v5*)
		AC_DEFINE(HAVE_SCO_ETC_SHADOW)
		no_shadow_password_checking=yes
		;;	
	*-convex-bsd*)
		no_shadow_password_checking=yes
		;;
	*-*-freebsd*)
		no_shadow_password_checking=yes
		;;
	*-*-netbsd*|*-*-openbsd*)
		no_shadow_password_checking=yes
		;;
	*)
		;;
esac

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_TIME
AC_C_BIGENDIAN
AC_CHECK_FUNCS(strchr memcpy setgroups flock)
AC_CHECK_HEADERS(fcntl.h limits.h sys/file.h syslog.h unistd.h shadow.h grp.h sys/time.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_OFF_T
AC_TYPE_UID_T
AC_TYPE_PID_T
AC_CHECK_TYPE(ssize_t, int)
AC_MSG_CHECKING(for socklen_t)
AC_TRY_COMPILE( [
	#include <sys/types.h>
	#include <sys/socket.h>], [
	socklen_t tmp;],[
	AC_MSG_RESULT(yes)], [
	AC_DEFINE([socklen_t], [int],
			  [Define to `int' if <sys/types.h> doesn't
			   define])
	AC_MSG_RESULT(no)])
# Simple AC_CHECK_TYPE doesn't work.

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_CHECK_FUNCS(vsnprintf snprintf)
AC_CHECK_FUNCS(gethostname mkdir rmdir utime strdup strerror strtol setsockopt)
AC_CHECK_FUNC(authenticate)
AC_CHECK_FUNC(inet_aton, [
	inet_aton_found=yes], [
	AC_CHECK_LIB(resolv, inet_aton, SPOPLIBS="$SPOPLIBS -lresolv") ] )

AC_CHECK_LIB(nsl, t_accept, SPOPLIBS="$SPOPLIBS -lnsl")
AC_CHECK_LIB(socket, socket, SPOPLIBS="$SPOPLIBS -lsocket")

dnl Check for maillock() - ripped from qpopper
AC_CHECK_LIB(mail, maillock, SPOPLIBS="$SPOPLIBS -lmail"; AC_DEFINE(MAILOCK))

dnl Check for maillock() in liblockfile too for Debian - araqnid
AC_CHECK_LIB(lockfile, maillock, SPOPLIBS="$SPOPLIBS -llockfile"; AC_DEFINE(MAILOCK))

dnl Checks for authentication methods (mostly ripped from SSH)
AC_CHECK_FUNC(crypt, [
	crypt_found=yes] )
if test -z "$crypt_found"; then
	AC_CHECK_LIB(crypt, crypt, [
		SPOPLIBS="$SPOPLIBS -lcrypt"
		crypt_found=yes] )
fi

if test -z "$getspnam_found"; then
	AC_CHECK_FUNC(getspnam, [
		getspnam_found=yes] )
	if test -z "$getspnam_found"; then
		AC_CHECK_LIB(sec, getspnam, [
			SPOPLIBS="$SPOPLIBS -lsec"
			getspnam_found=yes] )
	fi
fi
AC_CHECK_FUNC(getpwnam, [
	getpwnam_found=yes] )

if test -z "$no_libsun"; then
	if test -z "$getpwnam_found"; then
		AC_CHECK_LIB(sun, getpwnam, [
			getpwnam_found=yes
			LIBS="$LIBS -lsun"] )
	fi
fi
if test -z "$no_shadow_password_checking"; then
	AC_MSG_CHECKING(for shadow passwords)
	if test -f /etc/shadow; then
		if test "$ac_cv_header_shadow_h" = "yes"; then
			AC_DEFINE(HAVE_ETC_SHADOW)
			AC_MSG_RESULT(/etc/shadow)
			if test -z "$getspnam_found"; then	
				AC_CHECK_LIB(shadow, getspnam, SPOPLIBS="$SPOPLIBS -lshadow")
			fi
		else
			AC_MSG_RESULT(no)
		fi
	elif test -n "$os_sunos"; then
		AC_DEFINE(HAVE_ETC_SECURITY_PASSWD_ADJUNCT)
		AC_MSG_RESULT(/etc/security/passwd.adjunct)
	else
		AC_MSG_RESULT(no)
	fi
fi

AC_MSG_CHECKING(whether to use PAM)
AC_ARG_ENABLE(pam,
	[AS_HELP_STRING([--enable-pam],
					[use PAM to authenticate user])],
	[	if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)
		else
			if test -f /usr/include/security/pam_appl.h; then
				AC_DEFINE([HAVE_PAM], [],
						  [Define if you want to use PAM
						   to authenticate user.])
				SPOPLIBS="$SPOPLIBS -lpam -ldl"
				AC_MSG_RESULT(yes)
			else
				AC_MSG_RESULT(no)
			fi
		fi
	],
	AC_MSG_RESULT(no) )
AC_SUBST(SPOPLIBS)

POP_AUTH=""
APOP_O=""
POP_AUTH_1=""
AC_MSG_CHECKING(whether to support APOP)
AC_ARG_ENABLE(apop,
	[AS_HELP_STRING([--enable-apop],
					[compile server with APOP support])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)
		else
			AC_MSG_RESULT(yes)
			POP_AUTH="pop_auth"
			POP_AUTH_1="pop_auth.1"
			APOP_O="apop.o"
			AC_DEFINE([APOP], [],
					  [Define if you want to use APOP])
		fi
	],
	AC_MSG_RESULT(no) )
AC_SUBST(POP_AUTH)
AC_SUBST(APOP_O)
AC_SUBST(POP_AUTH_1)

MAILBOX_O="mailbox.o"
AC_MSG_CHECKING(whether to support mailbox)
AC_ARG_ENABLE(mailbox,
	[AS_HELP_STRING([--enable-mailbox],
					[compile server with mailbox support])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)
			MAILBOX_O=""
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE(MDMAILBOX)
		fi
	],
	[	AC_MSG_RESULT(yes)
		AC_DEFINE(MDMAILBOX)
	] )
AC_SUBST(MAILBOX_O)

MAILDIR_O="maildir.o"
AC_MSG_CHECKING(whether to support maildir)
AC_ARG_ENABLE(maildir,
	[AS_HELP_STRING([--enable-maildir],
					[compile server with maildir support])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)
			MAILDIR_O=""
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE(MDMAILDIR)
		fi
	],
	[	AC_MSG_RESULT(yes)
		AC_DEFINE(MDMAILDIR)
	] )
AC_SUBST(MAILDIR_O)

CONFIGFILE_O="configfile.o"
CONFIGSCAN_O="configscan.o"
AC_MSG_CHECKING(whether to support configuration file)
AC_ARG_ENABLE(configfile,
	[AS_HELP_STRING([--enable-configfile],
					[compile server with configuration file support])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)
			CONFIGFILE_O=""
			CONFIGSCAN_O=""
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE(CONFIGFILE)
		fi
	],
	[	AC_MSG_RESULT(yes)
		AC_DEFINE(CONFIGFILE)
	] )
AC_SUBST(CONFIGFILE_O)
AC_SUBST(CONFIGSCAN_O)

AC_MSG_CHECKING(whether to support message expiration)
AC_ARG_ENABLE(expire,
	[AS_HELP_STRING([--enable-expire],
					[compile server with message expiration support])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE(EXPIRATION)
		fi
	],
	[	AC_MSG_RESULT(yes)
		AC_DEFINE(EXPIRATION)
	] )

USERCONFIG_O="userconfig.o"
AC_MSG_CHECKING(whether to support user configuration)
AC_ARG_ENABLE(userconfig,
	[AS_HELP_STRING([--enable-userconfig],
					[compile server with user configuration support])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)
			USERCONFIG_O=""
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE(USERCONFIG)
		fi
	],
	[	AC_MSG_RESULT(yes)
		AC_DEFINE(USERCONFIG)
	] )
AC_SUBST(USERCONFIG_O)
AC_MSG_CHECKING(whether to support bulletins)
AC_ARG_ENABLE(bulletins,
	[AS_HELP_STRING([--enable-bulletins],
					[compile server with bulletins support])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE([BULLETINS], [],
					  [Define if you want to have
					   bulletins support])
		fi
	],
	AC_MSG_RESULT(no) )
AC_MSG_CHECKING(whether to build standalone version)
STANDALONE_O=""
AC_ARG_ENABLE(standalone,
	[AS_HELP_STRING([--enable-standalone],
					[compile standalone server])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)
		else
			STANDALONE_O="standalone.o"
			AC_MSG_RESULT(yes)
			AC_DEFINE([STANDALONE], [],
					  [Define if you want to compile
					   standalone server])
		fi
	],
	AC_MSG_RESULT(no) )
AC_SUBST(STANDALONE_O)
AC_MSG_CHECKING(whether to add support for LAST command)
AC_ARG_ENABLE(last,
	[AS_HELP_STRING([--enable-last],
					[compile server with LAST command support])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE([LASTCMD], [],
					  [Define if you want to have LAST
					   command support])
		fi
	],
	AC_MSG_RESULT(no) )
AC_MSG_CHECKING(whether to add support for user names mapping)
MAPPING_O=""
AC_ARG_ENABLE(mapping,
	[AS_HELP_STRING([--enable-mapping],
					[compile server with user names mapping support])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)			
		else
			MAPPING_O="mapping.o"
			AC_MSG_RESULT(yes)
			AC_DEFINE([MAPPING], [],
					  [Define if you want to have support
					   for user names mapping])
		fi
	],
	AC_MSG_RESULT(no) )
AC_SUBST(MAPPING_O)
AC_MSG_CHECKING(whether to add support for non-IP based virtual hosting)
AC_ARG_ENABLE(nonip,
	[AS_HELP_STRING([--enable-nonip],
					[compile server with non-IP based virtual hosting support])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)			
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE([NONIPVIRTUALS], [],
					  [Define if you want to have support
					   for non-IP virtual hosting])
		fi
	],
	AC_MSG_RESULT(no) )
AC_MSG_CHECKING(whether to add support for AllowRootLogin option)
AC_ARG_ENABLE(allowroot,
	[AS_HELP_STRING([--enable-allowroot],
					[compile server with "AllowRootLogin" option support])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)			
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE([ALLOWROOTLOGIN], [],
					  [Define if you want to have support
					   for "AllowRootLogin" option])
		fi
	],
	AC_MSG_RESULT(no) )
AC_MSG_CHECKING(whether to add support for CreateMailDrop option)
AC_ARG_ENABLE(createmail,
	[AS_HELP_STRING([--enable-createmail],
					[compile server with "CreateMailDrop" option support])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)			
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE([CREATEMAILDROP], [],
					  [Define if you want to have support
					   for "CreateMailDrop" option])
		fi
	],
	AC_MSG_RESULT(no) )
AC_MSG_CHECKING(whether to add support for IPv6 protocol)
AC_ARG_ENABLE(ipv6,
	[AS_HELP_STRING([--enable-ipv6],
					[compile server with IPv6 support])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)			
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE([SPIPV6], [],
					  [Define if you want to have support
					   for IPv6 protocol])
		fi
	],
	AC_MSG_RESULT(no) )
AC_MSG_CHECKING(whether to log resolved host name)
AC_ARG_ENABLE(resolve,
	[AS_HELP_STRING([--enable-resolve],
					[log resolved host name])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)			
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE([RESOLVE_HOSTNAME], [],
					  [Define if you want to have resolved
					   peer name in logs])
		fi
	],
	AC_MSG_RESULT(no) )
AC_MSG_CHECKING(whether to log "connect from x" message)
AC_ARG_ENABLE(connect,
	[AS_HELP_STRING([--enable-connect],
					[log "connect from x" message])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE([LOG_CONNECT], [],
					  [If defined server logs "connect"
					   message])
		fi
	],
	AC_MSG_RESULT(no) )
AC_MSG_CHECKING(whether to add extended logging)
AC_ARG_ENABLE(logextend,
	[AS_HELP_STRING([--enable-logextend],
					[compile server with extended logging support])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE([LOG_EXTEND], [],
					  [Define if you want to have extended
					   logging])
		fi
	], [
	AC_MSG_RESULT(no)
	] )
AC_MSG_CHECKING(whether to add support for statistics)
AC_ARG_ENABLE(statistics,
	[AS_HELP_STRING([--enable-statistics],
					[compile server with support for statistics])], [
		if test "$enableval" = "no"; then
			AC_MSG_RESULT(no)
		else
			AC_MSG_RESULT(yes)
			AC_DEFINE([STATISTICS], [],
					  [Define if you want to have support
					   for statistics])
		fi
	], [
	AC_MSG_RESULT(no)
	] )
	

AC_OUTPUT(Makefile doc/Makefile man/Makefile src/Makefile)

# vim: sw=4 ts=4 noet
