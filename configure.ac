# $Id: configure.ac,v 1.24 2004-09-22 06:27:26 victor Exp $
#
# Process this file with autoconf to produce a configure script.
#

AC_INIT([NetXMS], [0.1.0], [NetXMS Team <bugs@netxms.org>])
AC_CONFIG_AUX_DIR([config])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# --------------------------------------------------------------------------
# misc

AC_ARG_VAR(PERL,local path to the perl interpreter)
perl_possible_path="/usr/bin:/usr/local/bin:/bin:/opt/perl/bin:/opt/perl/usr/bin:/opt/perl/usr/local/bin"
AC_PATH_PROG(PERL,perl,/usr/bin/env perl,$perl_possible_path)

if test -d /usr/local/include; then
	CFLAGS="$CFLAGS -I/usr/local/include"
	CPPFLAGS="$CPPFLAGS -I/usr/local/include"
fi

if test -d /usr/kerberos/include; then
	CFLAGS="$CFLAGS -I/usr/kerberos/include"
	CPPFLAGS="$CPPFLAGS -I/usr/kerberos/include"
fi

if test -d /usr/local/ssl/include; then
	CFLAGS="$CFLAGS -I/usr/local/ssl/include"
	CPPFLAGS="$CPPFLAGS -I/usr/local/ssl/include"
fi

if test -d /usr/local/lib; then
	LDFLAGS="$LDFLAGS -L/usr/local/lib"
fi

# --------------------------------------------------------------------------
# command line options

AC_ARG_WITH(mysql,
[AS_HELP_STRING(--with-mysql,enable mysql backend)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
		LDFLAGS="$LDFLAGS -L${withval}/lib -L${withval}/lib/mysql -L${withval}/mysql/lib"
		CPPFLAGS="$CPPFLAGS -I${withval}/include -I${withval}/include/mysql -I${withval}/mysql/include"
	else
		LD_RUN_PATH="/usr/local/mysql/lib:/usr/local/mysql/lib/mysql:/usr/lib/mysql:/usr/mysql/lib:/usr/mysql/lib/mysql:/usr/local/lib/mysql:${LD_RUN_PATH:+:}${LD_RUN_PATH}"
		LDFLAGS="$LDFLAGS -L/usr/local/mysql/lib -L/usr/lib/mysql -L/usr/mysql/lib -L/usr/local/lib/mysql -L/usr/local/mysql/lib/mysql -L/usr/mysql/lib/mysql"
		CPPFLAGS="$CPPFLAGS -I/usr/local/mysql/include -I/usr/include/mysql -I/usr/mysql/include -I/usr/local/include/mysql -I/usr/local/mysql/include/mysql -I/usr/mysql/include/mysql"
	fi
	AC_CHECK_LIB(m, floor)
	AC_CHECK_LIB(z, gzclose)
	with_mysql="yes"
	AC_DEFINE(WITH_MYSQL,,[with mysql])
	DB_DRIVERS="$DB_DRIVERS mysql"
	AC_CHECK_LIB(mysqlclient, mysql_init, ,
		[AC_MSG_ERROR(libmysqlclient is needed for MySQL support)])
	AC_MSG_CHECKING(whether mysql clients can run)
	AC_RUN_IFELSE([AC_LANG_SOURCE([[
		#include <stdio.h>
		#include <mysql.h>    
		int main(void)
		{
			MYSQL *a = mysql_init(NULL);
			return 0;
		}
		]])],[],[
			AC_MSG_RESULT(no)
			AC_MSG_ERROR(Your MySQL client libraries aren't properly installed)
	],[])
	AC_MSG_RESULT(yes)
	AC_CHECK_FUNCS(mysql_real_escape_string)
fi ])

AC_ARG_WITH(pgsql,
[AS_HELP_STRING(--with-pgsql,enable postgres backend)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"    
		LDFLAGS="$LDFLAGS -L${withval}/lib -L${withval}/lib/pgsql -L${withval}/lib/postgresql -L${withval}/pgsql/lib -L${withval}/postgresql/lib"
		CPPFLAGS="$CPPFLAGS -I${withval}/include -I${withval}/include/pgsql -I${withval}/include/postgresql -I${withval}/pgsql/include -I${withval}/postgresql/include"
	else
		LD_RUN_PATH="/usr/local/pgsql/lib:/usr/local/pgsql/lib/pgsql:/usr/lib/pgsql:/usr/pgsql/lib:/usr/pgsql/lib/pgsql:/usr/local/lib/pgsql:/usr/local/postgresql/lib:/usr/local/postgresql/lib/postgresql:/usr/lib/postgresql:/usr/postgresql/lib:/usr/postgresql/lib/postgresql:/usr/local/lib/postgresql:${LD_RUN_PATH:+:}${LD_RUN_PATH}"
		LDFLAGS="$LDFLAGS -L/usr/local/pgsql/lib -L/usr/lib/pgsql -L/usr/pgsql/lib -L/usr/local/lib/pgsql -L/usr/local/pgsql/lib/pgsql -L/usr/pgsql/lib/pgsql -L/usr/local/postgresql/lib -L/usr/lib/postgresql -L/usr/postgresql/lib -L/usr/local/lib/postgresql -L/usr/local/postgresql/lib/postgresql -L/usr/postgresql/lib/postgresql"
		CPPFLAGS="$CPPFLAGS -I/usr/local/pgsql/include -I/usr/include/pgsql -I/usr/pgsql/include -I/usr/local/include/pgsql -I/usr/local/pgsql/include/pgsql -I/usr/pgsql/include/pgsql -I/usr/local/postgresql/include -I/usr/include/postgresql -I/usr/postgresql/include -I/usr/local/include/postgresql -I/usr/local/postgresql/include/postgresql -I/usr/postgresql/include/postgresql"
	fi
	AC_CHECK_LIB(m, floor)
	AC_CHECK_LIB(z, gzclose)
	with_pgsql="yes"
	DB_DRIVERS="$DB_DRIVERS pgsql"
	AC_DEFINE(WITH_PGSQL,,[with pgsql])
	AC_CHECK_LIB(pq, PQconnectdb, ,
		[AC_MSG_ERROR(libpq is needed for PostgreSQL support)])        
			AC_MSG_CHECKING(whether postgresql clients can run)
			AC_RUN_IFELSE([AC_LANG_SOURCE([[
				#include <stdio.h>
				#include <libpq-fe.h>
				int main(void)
				{
					PGconn *a = PQconnectdb("");
					return 0;
				}
			]])],[],[
			AC_MSG_RESULT(no)
			AC_MSG_ERROR(Your PostgreSQL client libraries aren't properly installed)
		],[])    
	AC_MSG_RESULT(yes)
fi ])

AC_SUBST(DB_DRIVERS)


# --------------------------------------------------------------------------
# checks for programs.

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL

AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

# --------------------------------------------------------------------------
# checks for libs.

#AC_CHECK_LIB([socket], [socket])
#AC_CHECK_LIB([nsl], [gethostbyname])

AC_CHECK_LIB(crypto, DH_new)
AC_CHECK_LIB(ssl, SSL_accept)
AC_CHECK_LIB(termcap, tgetstr, [], [AC_CHECK_LIB(ncurses, tgetstr, [], AC_CHECK_LIB(curses, tgetstr))])
AC_CHECK_LIB(readline, readline)
AC_CHECK_LIB(socket, if_nameindex)

# --------------------------------------------------------------------------
# checks for headers.

AC_HEADER_STDC
AC_HEADER_STAT
AC_HEADER_TIME
AC_CHECK_HEADERS([sys/int_types.h])
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h sys/socket.h])
AC_CHECK_HEADERS([sys/ioctl.h sys/sockio.h])
AC_CHECK_HEADERS([net/if.h net/if_arp.h net/if_dl.h])
AC_CHECK_HEADERS([openssl/ssl.h])
AC_CHECK_HEADERS([readline/readline.h])
AC_CHECK_HEADERS([byteswap.h])
AC_CHECK_HEADERS([sys/mman.h])
AC_CHECK_HEADERS([sys/utsname.h])

if test "x$ac_cv_header_openssl_ssl_h" != "xyes" ; then
	AC_MSG_ERROR(OpenSSL headers not found.)
fi

# --------------------------------------------------------------------------
# types
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)

AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_STRUCT_TIMEZONE

AC_CHECK_TYPES([int64_t, uint64_t, u_int64_t])
AC_CHECK_TYPES([off_t])

# --------------------------------------------------------------------------
# C/CPP caps

AC_PROG_GCC_TRADITIONAL
AC_C_CONST
AC_C_BIGENDIAN

# --------------------------------------------------------------------------
# macros

AC_CHECK_DECLS([__bswap_64],,,[
#if HAVE_BYTESWAP_H
#include <byteswap.h>
#endif
])
AC_CHECK_DECLS([SIOCGIFADDR, SIOCGIFNETMASK, SIOCGIFHWADDR],,,[
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
])

# --------------------------------------------------------------------------
# library functions

AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF

AC_CHECK_FUNCS([gettimeofday memmove memset strchr strcspn strdup strerror strrchr strtol strtoul])
AC_CHECK_FUNCS([if_nametoindex daemon mmap strerror_r])

# --------------------------------------------------------------------------
# UNICODE support

AC_CHECK_HEADERS([wchar.h])
AC_CHECK_TYPES([wchar_t])
AC_CHECK_FUNCS([wctomb mbtowc wcslen])


# sockets/resolver (probably for solaris)
AC_CHECK_FUNC(connect, , [AC_CHECK_LIB(socket, connect)])
AC_CHECK_FUNC(gethostbyname, , [AC_CHECK_LIB(resolv, gethostbyname)])
AC_CHECK_FUNC(gethostbyname, , [AC_CHECK_LIB(nsl, gethostbyname)])

if test "x$ac_cv_lib_nsl_gethostbyname" != "xyes" && test "x$ac_cv_func_gethostbyname" != "xyes" ; then
	AC_CHECK_FUNC(gethostbyname, , [AC_CHECK_LIB(socket, gethostbyname)])
fi

if test "$ac_cv_lib_nsl_gethostbyname" = "$ac_cv_func_gethostbyname" ; then
	AC_MSG_CHECKING([if we can include libnsl + libsocket])
	LIBS="-lnsl -lsocket $LIBS"
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[(void) gethostbyname]])],[my_ac_link_result=yes],[my_ac_link_result=no ])
	if test "$my_ac_link_result" = "no" ; then
		AC_MSG_RESULT([failure])
		AC_MSG_ERROR([unable to use gethostbyname()])
	else
		AC_MSG_RESULT([success])
	fi
fi

# posix threads
AC_CHECK_HEADER(pthread.h,,AC_MSG_ERROR([*** POSIX thread support not installed - please install first ***]))

PTHREAD_LIBS=error
AC_MSG_CHECKING(for old style FreeBSD -pthread flag)
AC_EGREP_CPP(yes,
	[#if (defined(__FreeBSD_cc_version) && __FreeBSD_cc_version <= 500001) || defined(__OpenBSD__)
		yes
	#endif
	], AC_MSG_RESULT(yes)
	CPPFLAGS="$CPPFLAGS -D_THREAD_SAFE" PTHREAD_LIBS="-pthread",
	AC_MSG_RESULT(no))
if test "x$PTHREAD_LIBS" = xerror; then
	AC_CHECK_LIB(pthread, pthread_attr_init,
		PTHREAD_LIBS="-lpthread")
fi
if test "x$PTHREAD_LIBS" = xerror; then
	AC_CHECK_LIB(pthreads, pthread_attr_init,
		PTHREAD_LIBS="-lpthreads")
fi
if test "x$PTHREAD_LIBS" = xerror; then
	AC_CHECK_LIB(c_r, pthread_attr_init,
		PTHREAD_LIBS="-lc_r")
fi
if test "x$PTHREAD_LIBS" = xerror; then
	AC_CHECK_FUNC(pthread_attr_init,
		PTHREAD_LIBS="")
fi
if test "x$PTHREAD_LIBS" = xerror; then
	AC_MSG_ERROR(*** Unable to locate working posix thread library ***)
fi
AC_SUBST(PTHREAD_LIBS)

# Extensions to posix threads
AC_CHECK_FUNCS([pthread_cond_reltimedwait_np])

# --------------------------------------------------------------------------
# OS type
case `uname -s` in
	Linux)
		SUBAGENT_DIRS="linux"
		;;
	FreeBSD)
		SUBAGENT_DIRS="freebsd"
		;;
	SunOS)
		SUBAGENT_DIRS="sunos"
		;;
	*)
		# unknown
		;;
esac
AC_SUBST(SUBAGENT_DIRS)

# --------------------------------------------------------------------------

# shared libs versions
#
# versioning scheme: current:revision:age
#
# current
#	The number of the current interface exported by the library. A current
#	value of `0', means that you are calling the interface exported by this
#	library interface 0.
#
# revision
#	The implementation number of the most recent interface exported by this
#	library. In this case, a revision value of `0' means that this is the
#	first implementation of the interface.
#	If the next release of this library exports the same interface, but has a
#	different implementation (perhaps some bugs have been fixed), the revision
#	number will be higher, but current number will be the same. In that case,
#	when given a choice, the library with the highest revision will always
#	be used by the runtime loader.
#
# age
#	The number of previous additional interfaces supported by this library.
#	If age were `2', then this library can be linked into executables which
#	were built with a release of this library that exported the current
#	interface number, current, or any of the previous two interfaces.
#	By definition age must be less than or equal to current. At the outset,
#	only the first ever interface is implemented, so age can only be `0'. 
#
#
#

LIBNETXMS_LIBRARY_VERSION=0:1:0
LIBNXCSCP_LIBRARY_VERSION=0:1:0
LIBNXSRV_LIBRARY_VERSION=0:1:0
AC_SUBST(LIBNETXMS_LIBRARY_VERSION)
AC_SUBST(LIBNXCSCP_LIBRARY_VERSION)
AC_SUBST(LIBNXSRV_LIBRARY_VERSION)

# --------------------------------------------------------------------------

AC_CONFIG_FILES([
   Makefile
   README
   doc/Makefile
   m4/Makefile
   sql/Makefile
   include/Makefile
   src/Makefile
   src/libnetxms/Makefile
   src/libnxcscp/Makefile
   src/libnxcl/Makefile
   src/server/Makefile
   src/server/libnxsrv/Makefile
   src/server/core/Makefile
   src/server/tools/Makefile
   src/server/tools/nxget/Makefile
   src/server/tools/nxaction/Makefile
   src/server/dbdrv/Makefile
   src/server/dbdrv/mysql/Makefile
   src/server/dbdrv/pgsql/Makefile
   src/agent/Makefile
   src/agent/core/Makefile
   src/agent/subagents/Makefile
   src/agent/subagents/skeleton/Makefile
   src/agent/subagents/linux/Makefile
])

AC_OUTPUT
