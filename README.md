# Solid POP3 Server

## DEPRECATION NOTICE

**This particular Git repository is not being maintained.** It contains a
number of improvements over the last upstream release (which is 0.15). See file
[ChangeLog](ChangeLog) for a detailed list.

If you are looking for a POP3 server, I recommend
[pop3d from GNU Mailutils](https://mailutils.org/manual/html_section/pop3d.html#pop3d).


## GENERAL INFORMATION

The Solid POP3 Server is an implementation of a Post Office Protocol version 3
server that has flexibility as its main goal. The server is easily
configurable and has support for few features such as:

- APOP authentication scheme
- virtual hosting
- maildir and mailbox handling
- bulletins
- expiration of messages

Each user can specify his maildrop (its position and type). The format
used in specification of maildrop's position should handle almost all
widely-used system configurations. The server also seems to be fast, however no
tests have been performed, so it's rather relative feeling. The design used
is very similar to the design of Solar Designer's POPA3D server. This solution
let's minimalize size of code working with root privileges. The code was also
heavily checked for buffer overflow leaks and file races. None have been found
as for now. All operations on files are done with user privileges. There
is no `SUID` APOP secrets database management program (as in QPOP). Each user
can specify his secret in his own home directory.

Default maildrop name is `/var/spool/mail/%s` (read `spop3d(8)` manual).
Most Linux distributions work with this setting. However on other systems
you should change this value (use `-n` option or edit `src/const.h`).

The newest version of the server is available under:

- <ftp://ftp.rudykot.pl/pub/solidpop3d>
- <ftp://sedez.iq.pl/pub/solidpop3d>
- <ftp://dione.ids.pl/pub/solidpop3d>

Homepage:
    http://solidpop3d.pld.org.pl/

Any suggestions, bug reports, information about successful ports should go to:
    Jerzy Balamut, <jurekb@dione.ids.pl>


## INSTALLING

Consult `INSTALL` file for generic instructions about installing.

Existance of the user `spop3d` in system is required. Big part of the server
works with this user privileges. Following `./configure` options are recognized:

Option                  | Description
----------------------- |---------------------------------------------------------------------------
`--enable-pam`          | add PAM support
`--enable-apop`         | add APOP authentication scheme support
`--enable-mailbox`      | add mailbox handling support
`--enable-maildir`      | add maildir handling support
`--enable-bulletins`    | add bulletins support
`--enable-expire`       | add support for message expiration
`--enable-standalone`   | compile server as a standalone server, not inetd server (which is default)
`--enable-configfile`   | add support for global configuration file
`--enable-userconfig`   | add support for user configuration file
`--enable-last`         | add support for `LAST` command
`--enable-mapping`      | add support for user names mapping
`--enable-nonip`        | add support for non-IP based virtuals
`--enable-allowroot`    | add support for `AllowRootLogin` option
`--enable-createmail`   | add support for `CreateMailDrop` option
`--enable-ipv6`         | add support for IPv6 protocol
`--enable-resolve`      | log resolved host name with IP number
`--enable-connect`      | log `connect from xxx` message
`--enable-logextend`    | log peer IP number in few additional places

Support for mailbox, maildir, expiration, configfile and userconfig 
is enabled by default. You can change in the file `src/const.h` default
values for some options.

**WARNING:** The global configuration file should be readable by the `spop3d` user!!!

Simple installation process could look like this:

```
$ ./configure
$ make
$ su
# useradd -d /nonexistent -s /nonexistent -M spop3d
# make install
# echo pop3 stream tcp nowait root /usr/sbin/tcpd /usr/local/sbin/spop3d >> /etc/inetd.conf
# killall -HUP inetd
```

**WARNING:** On some systems you should replace `pop3` with `pop-3`.


## BULLETINS

Server looks for bulletins in directory `${localstatedir}/bulletins` (default:
`/usr/local/var/bulletins`). For each bulletin (file) its modification time
is checked. If this modification time is more recent than the modification time
of the file `UserBullFile` this bulletin is added to user's maildrop. Server
touches the modification time of the file `UserBullFile` after all bulletins
are checked. The directory `${localstatedir}` and bulletins should be accessable
by any user. Default value for `UserBullFile` is `.spop3d-bull`. Read
`spop3d.conf(5)` manual for details.


## APOP

Warning: Use APOP only if you really need it (and probably you don't).
It isn't such secure as it looks.


## COPYRIGHT

The program is distributed under GNU General Public License.
See the file `COPYING` for details.

____________________________________
Jerzy Balamut, <jurekb@dione.ids.pl>


<!-- vim: se tw=79 cc=+1 et sw=4 ts=4: -->
