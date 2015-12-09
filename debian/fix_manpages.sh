#!/bin/sh

set -e 

cd "$1"
cd usr/share/man

for man_page in man*/* ; do
	sed  -e 's|var/spool/mail|var/mail|g' \
	     -e 's| spop3d | solid-pop3d |g' \
	     -e 's|^spop3d |solid-pop3d |g' \
	     -e 's|\\fBspop3d\\fP|\\fBsolid-pop3d\\fP|g' \
	     -e 's| spop3d$| solid-pop3d|g' \
	     -e 's|\${sysconfdir}|/etc|g' \
	     -e 's|\${localstatedir}|/var/lib/solid-pop3d|g' \
		< "$man_page" > tmp
	     touch -r "$man_page" tmp
	     mv -f tmp "$man_page"
done
