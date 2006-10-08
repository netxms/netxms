# $Id: configure.ac,v 1.198 2006-10-08 16:46:58 victor Exp $
#
# Process this file with autoconf to produce a configure script.
#

AC_INIT([NetXMS], [0.2.14-rc3], [NetXMS Team <bugs@netxms.org>])
AC_CONFIG_AUX_DIR([config])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

ALL_STATIC_FLAG=""

# --------------------------------------------------------------------------
# checks for programs.

AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_LN_S

# --------------------------------------------------------------------------
# Misc. settings
CFLAGS="$CFLAGS -D_POSIX_PTHREAD_SEMANTICS -D_XOPEN_SOURCE=500 -D_XOPEN_SOURCE_EXTENDED=1 -D_THREAD_SAFE"
CPPFLAGS="$CPPFLAGS -D_POSIX_PTHREAD_SEMANTICS -D_XOPEN_SOURCE=500 -D_XOPEN_SOURCE_EXTENDED=1 -D_THREAD_SAFE"
SUBAGENT_LIBS=""

# --------------------------------------------------------------------------
# disable C++ exceptions

AC_LANG_PUSH([C++])

AC_MSG_CHECKING(whether C++ compiler accepts -fno-rtti)
OLD_CXXFLGAS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -fno-rtti"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
	[ AC_MSG_RESULT(yes) ], 
	[ 
		CXXFLAGS="$OLD_CXXFLAGS"
		AC_MSG_RESULT(no)
	])

AC_MSG_CHECKING(whether C++ compiler accepts -fno-exceptions)
OLD_CXXFLGAS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -fno-exceptions"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
	[ AC_MSG_RESULT(yes) ], 
	[ 
		CXXFLAGS="$OLD_CXXFLAGS"
		AC_MSG_RESULT(no)
	])

AC_LANG_POP([C++])

# --------------------------------------------------------------------------
# set LDFLAGS depending on OS

case `uname -s` in
   AIX)
      LDFLAGS="-no-undefined $LDFLAGS"
      ;;
   *)
      ;;
esac

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

if test -d /usr/local/lib; then
	LDFLAGS="$LDFLAGS -L/usr/local/lib"
fi

# --------------------------------------------------------------------------
# check for zlib

AC_CHECK_LIB(z, deflate)

# --------------------------------------------------------------------------
# command line options

AC_ARG_WITH(sqlite,
[AS_HELP_STRING(--with-sqlite,build SQLite database driver)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
		LDFLAGS="$LDFLAGS -L${withval}/lib"
		CPPFLAGS="$CPPFLAGS -I${withval}/include"
   fi
   DYNAMIC_COMPONENTS="yes"
	with_sqlite="yes"
	AC_DEFINE(WITH_SQLITE,,[with sqlite])
	DB_DRIVERS="$DB_DRIVERS sqlite"
	AC_CHECK_LIB(sqlite3, sqlite3_open, ,
		[AC_MSG_ERROR(libsqlite3 is required for SQLite support)])
fi ])

AC_ARG_WITH(odbc,
[AS_HELP_STRING(--with-odbc,build ODBC database driver)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
		LDFLAGS="$LDFLAGS -L${withval}/lib"
		CPPFLAGS="$CPPFLAGS -I${withval}/include"
   fi
   DYNAMIC_COMPONENTS="yes"
	with_odbc="yes"
	AC_DEFINE(WITH_ODBC,,[with odbc])
	DB_DRIVERS="$DB_DRIVERS odbc"
	AC_CHECK_LIB(odbc, SQLAllocHandle, ,
		[AC_MSG_ERROR(libodbc is required for ODBC support)])
fi ])

AC_ARG_WITH(mysql,
[AS_HELP_STRING(--with-mysql,build MySQL database driver)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
		LDFLAGS="$LDFLAGS -L${withval}/lib -L${withval}/lib/mysql -L${withval}/mysql/lib"
		CPPFLAGS="$CPPFLAGS -I${withval}/include -I${withval}/include/mysql -I${withval}/mysql/include"
	else
		LD_RUN_PATH="/usr/local/mysql/lib:/usr/local/mysql/lib/mysql:/usr/lib64/mysql:/usr/lib/mysql:/usr/mysql/lib:/usr/mysql/lib/mysql:/usr/local/lib/mysql:${LD_RUN_PATH:+:}${LD_RUN_PATH}"
		LDFLAGS="$LDFLAGS -L/usr/local/mysql/lib -L/usr/lib64/mysql -L/usr/lib/mysql -L/usr/mysql/lib -L/usr/local/lib/mysql -L/usr/local/mysql/lib/mysql -L/usr/mysql/lib/mysql"
		CPPFLAGS="$CPPFLAGS -I/usr/local/mysql/include -I/usr/include/mysql -I/usr/mysql/include -I/usr/local/include/mysql -I/usr/local/mysql/include/mysql -I/usr/mysql/include/mysql"
	fi
   DYNAMIC_COMPONENTS="yes"
	AC_CHECK_LIB(m, floor)
	with_mysql="yes"
	AC_DEFINE(WITH_MYSQL,,[with mysql])
	DB_DRIVERS="$DB_DRIVERS mysql"
	AC_CHECK_LIB(mysqlclient, mysql_init, ,
		[AC_MSG_ERROR(libmysqlclient is requred for MySQL support)])
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
[AS_HELP_STRING(--with-pgsql,build PostgreSQL database driver)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"    
		LDFLAGS="$LDFLAGS -L${withval}/lib -L${withval}/lib/pgsql -L${withval}/lib/postgresql -L${withval}/pgsql/lib -L${withval}/postgresql/lib"
		CPPFLAGS="$CPPFLAGS -I${withval}/include -I${withval}/include/pgsql -I${withval}/include/postgresql -I${withval}/pgsql/include -I${withval}/postgresql/include"
	else
		LD_RUN_PATH="/usr/local/pgsql/lib:/usr/local/pgsql/lib/pgsql:/usr/lib64/pgsql:/usr/lib/pgsql:/usr/pgsql/lib:/usr/pgsql/lib/pgsql:/usr/local/lib/pgsql:/usr/local/postgresql/lib:/usr/local/postgresql/lib/postgresql:/usr/lib/postgresql:/usr/postgresql/lib:/usr/postgresql/lib/postgresql:/usr/local/lib/postgresql:${LD_RUN_PATH:+:}${LD_RUN_PATH}"
		LDFLAGS="$LDFLAGS -L/usr/local/pgsql/lib -L/usr/lib64/pgsql -L/usr/lib/pgsql -L/usr/pgsql/lib -L/usr/local/lib/pgsql -L/usr/local/pgsql/lib/pgsql -L/usr/pgsql/lib/pgsql -L/usr/local/postgresql/lib -L/usr/lib/postgresql -L/usr/postgresql/lib -L/usr/local/lib/postgresql -L/usr/local/postgresql/lib/postgresql -L/usr/postgresql/lib/postgresql"
		CPPFLAGS="$CPPFLAGS -I/usr/local/pgsql/include -I/usr/include/pgsql -I/usr/pgsql/include -I/usr/local/include/pgsql -I/usr/local/pgsql/include/pgsql -I/usr/pgsql/include/pgsql -I/usr/local/postgresql/include -I/usr/include/postgresql -I/usr/postgresql/include -I/usr/local/include/postgresql -I/usr/local/postgresql/include/postgresql -I/usr/postgresql/include/postgresql"
	fi
   DYNAMIC_COMPONENTS="yes"
	AC_CHECK_LIB(m, floor)
	with_pgsql="yes"
	DB_DRIVERS="$DB_DRIVERS pgsql"
	AC_DEFINE(WITH_PGSQL,,[with pgsql])
	AC_CHECK_LIB(pq, PQconnectdb, ,
		[AC_MSG_ERROR(libpq is required for PostgreSQL support)])        
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

AC_ARG_WITH(server,
[AS_HELP_STRING(--with-server,build server)],
[
	MODULES="$MODULES libnxmap libnxsnmp libnxsl server nxmibc nxscript"
   TOP_LEVEL_MODULES="$TOP_LEVEL_MODULES sql images"
   CONTRIB_MODULES="$CONTRIB_MODULES mibs backgrounds"
	BUILD_SERVER="yes"
   DYNAMIC_COMPONENTS="yes"
])


AC_ARG_WITH(client,
[AS_HELP_STRING(--with-client,build client library and tools)],
[
	MODULES="$MODULES libnxmap libnxcl nxevent nxreport console"
	BUILD_CLIENT="yes"
   DYNAMIC_COMPONENTS="yes"
])


AC_ARG_WITH(mdebug,
[AS_HELP_STRING(--with-mdebug,use debug version of memory allocation functions)],
[
	CFLAGS="$CFLAGS -DNETXMS_MEMORY_DEBUG"
	CPPFLAGS="$CPPFLAGS -DNETXMS_MEMORY_DEBUG"
])


SUBAGENT_DIRS=""
AC_ARG_WITH(agent,
[AS_HELP_STRING(--with-agent,build agent)],
[
	BUILD_AGENT=yes
	MODULES="$MODULES agent"
	case `uname -s` in
		Linux)
			SUBAGENT_DIRS="linux"
			;;
		FreeBSD)
			SUBAGENT_DIRS="freebsd"
			;;
		OpenBSD)
			SUBAGENT_DIRS="openbsd"
			;;
		SunOS)
			SUBAGENT_DIRS="sunos"
			;;
		AIX)
			SUBAGENT_DIRS="aix"
			;;
		HP-UX)
			SUBAGENT_DIRS="hpux"
			;;
		*)
			# unknown
			;;
	esac
   DYNAMIC_COMPONENTS="yes"
])

AC_ARG_WITH(static-agent,
[AS_HELP_STRING(--with-static-agent,build statically linked agent)],
[
   BUILD_STATIC_AGENT="yes"
   MODULES="$MODULES agent"
   CFLAGS="$CFLAGS -D_STATIC_AGENT"
   CPPFLAGS="$CPPFLAGS -D_STATIC_AGENT"
   LDFLAGS="$LDFLAGS"
   ALL_STATIC_FLAG="-all-static"
   #STATIC_SUBAGENT_LIST="ping portcheck"
	case `uname -s` in
		Linux)
			SUBAGENT_DIRS="linux"
			SUBAGENT_LIBS="../subagents/linux/libnsm_linux.la"
			STATIC_SUBAGENT_LIST="linux $STATIC_SUBAGENT_LIST"
			;;
		FreeBSD)
			SUBAGENT_DIRS="freebsd"
			SUBAGENT_LIBS="../subagents/freebsd/libnsm_freebsd.la"
			STATIC_SUBAGENT_LIST="freebsd $STATIC_SUBAGENT_LIST"
			;;
		OpenBSD)
			SUBAGENT_DIRS="openbsd"
			SUBAGENT_LIBS="../subagents/openbsd/libnsm_openbsd.la"
			STATIC_SUBAGENT_LIST="openbsd $STATIC_SUBAGENT_LIST"
			;;
		SunOS)
			SUBAGENT_DIRS="sunos"
			SUBAGENT_LIBS="../subagents/sunos/libnsm_sunos.la"
			STATIC_SUBAGENT_LIST="sunos $STATIC_SUBAGENT_LIST"
			;;
		AIX)
			SUBAGENT_DIRS="aix"
			SUBAGENT_LIBS="../subagents/aix/libnsm_aix.la"
			STATIC_SUBAGENT_LIST="aix $STATIC_SUBAGENT_LIST"
			;;
		HP-UX)
			SUBAGENT_DIRS="hpux"
			SUBAGENT_LIBS="../subagents/hpux/libnsm_hpux.la"
			STATIC_SUBAGENT_LIST="hpux $STATIC_SUBAGENT_LIST"
			;;
		*)
			# unknown
			;;
	esac
])

AC_ARG_WITH(ipso-agent,
[AS_HELP_STRING(--with-ipso-agent,build statically linked IPSO agent)],
[
   BUILD_STATIC_AGENT="yes"
   MODULES="$MODULES agent"
   CFLAGS="$CFLAGS -D_STATIC_AGENT -D_IPSO"
   CPPFLAGS="$CPPFLAGS -D_STATIC_AGENT -D_IPSO"
   LDFLAGS="$LDFLAGS -all-static"
   STATIC_SUBAGENT_LIST="ipso ping portcheck ups"
   SUBAGENT_DIRS="ipso"
   SUBAGENT_LIBS="../subagents/ipso/libnsm_ipso.la ../subagents/ping/libnsm_ping.la ../subagents/portCheck/libnsm_portCheck.la ../subagents/ups/libnsm_ups.la"
])

UNSTABLE_SUBAGENT_DIRS=""
AC_ARG_ENABLE(unstable,
[AS_HELP_STRING(--enable-unstable,build experimental/unstable components)],
[
	UNSTABLE_SUBAGENT_DIRS="logscan"
])

AC_ARG_WITH(dist,
[AS_HELP_STRING(--with-dist,for maintainers only)],
	DB_DRIVERS="mysql pgsql odbc mssql sqlite"
	MODULES="libnxsnmp libnxsl server agent libnxmap libnxcl nxevent nxreport nxmibc console nxhttpd nxscript"
	SUBAGENT_DIRS="linux freebsd openbsd sunos aix ipso hpux"
	NXCONFIG="nxconfig"
   TOP_LEVEL_MODULES="sql images"
   CONTRIB_MODULES="mibs backgrounds"
	UNSTABLE_SUBAGENT_DIRS="logscan"
)

AC_ARG_WITH(nxhttpd,
[AS_HELP_STRING(--with-nxhttpd,build web interface)],
[
	if test "x$withval" != "xno" ; then
		MODULES="$MODULES libnxmap libnxcl nxhttpd"
		BUILD_WEB_CLIENT="yes"
		DYNAMIC_COMPONENTS="yes"
	fi
])

AC_ARG_WITH(openssl,
	[AS_HELP_STRING(--with-openssl,specify OpenSSL location)],
	[
		if test "x$withval" != "xno" ; then
			if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
				LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
				LDFLAGS="$LDFLAGS -L${withval}/lib"
				CPPFLAGS="$CPPFLAGS -I${withval}/include"
			else
				LD_RUN_PATH="/usr/local/lib:/opt/openssl/lib:/usr/local/ssl/lib:${LD_RUN_PATH:+:}${LD_RUN_PATH}"
				LDFLAGS="$LDFLAGS -L/usr/local/lib -L/opt/openssl/lib"
				CPPFLAGS="$CPPFLAGS -I/usr/local/include -I/opt/openssl/include"
			fi
			AC_CHECK_LIB(crypto, RSA_new,
				[
				 	AC_DEFINE(WITH_OPENSSL,,[with openssl])
					AC_CHECK_LIB(crypto, EVP_aes_256_cbc,
									 [], [AC_DEFINE(NETXMS_NO_AES,,[desc])])
					AC_CHECK_LIB(crypto, EVP_bf_cbc,
									 [], [AC_DEFINE(NETXMS_NO_BF,,[desc])])
					AC_CHECK_LIB(crypto, EVP_idea_cbc,
									 [], [AC_DEFINE(NETXMS_NO_IDEA,,[desc])])
					AC_CHECK_LIB(crypto, EVP_des_ede3_cbc,
									 [], [AC_DEFINE(NETXMS_NO_DES,,[desc])])
				],
				[AC_MSG_ERROR(libcrypto is required for encryption support)])
		fi
	],
	[
		AC_CHECK_LIB(crypto, RSA_new,
			[
				AC_DEFINE(WITH_OPENSSL,,[with openssl])
				AC_CHECK_LIB(crypto, EVP_aes_256_cbc,
								 [], [AC_DEFINE(NETXMS_NO_AES,,[desc])])
				AC_CHECK_LIB(crypto, EVP_bf_cbc,
								 [], [AC_DEFINE(NETXMS_NO_BF,,[desc])])
				AC_CHECK_LIB(crypto, EVP_idea_cbc,
								 [], [AC_DEFINE(NETXMS_NO_IDEA,,[desc])])
				AC_CHECK_LIB(crypto, EVP_des_ede3_cbc,
								 [], [AC_DEFINE(NETXMS_NO_DES,,[desc])])
			],
			[AC_MSG_RESULT(libcrypto is required for encryption support)])
	 ])

AC_SUBST(DB_DRIVERS)
AC_SUBST(MODULES)
AC_SUBST(SUBAGENT_DIRS)
AC_SUBST(NXCONFIG)
AC_SUBST(TOP_LEVEL_MODULES)
AC_SUBST(CONTRIB_MODULES)
AC_SUBST(STATIC_SUBAGENT_LIST)
AC_SUBST(SUBAGENT_LIBS)
AC_SUBST(BUILD_SERVER)
AC_SUBST(UNSTABLE_SUBAGENT_DIRS)

# --------------------------------------------------------------------------
# If --with-static-agent selected, other components cannot be selected
# Select shared or static libraries depending on agent build type
if test "x$BUILD_STATIC_AGENT" = "xyes" ; then
   if test "x$DYNAMIC_COMPONENTS" = "xyes" ; then
      AC_MSG_ERROR(Static agent and other components are mutally exclusive.)
   fi
   enable_shared=no
else
   enable_static=no
fi

AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

# --------------------------------------------------------------------------
# checks for libs.

#AC_CHECK_LIB([socket], [socket])
#AC_CHECK_LIB([nsl], [gethostbyname])

AC_CHECK_LIB(socket, if_nameindex)
AC_CHECK_LIB(dl, dlopen)
AC_CHECK_LIB(kstat, kstat_open)

if test "x$BUILD_SERVER" = "xyes" ; then
   AC_CHECK_LIB(termcap, tgetstr, [], [AC_CHECK_LIB(ncurses, tgetstr, [], AC_CHECK_LIB(curses, tgetstr))])
   AC_CHECK_LIB(readline, readline)
fi

# --------------------------------------------------------------------------
# checks for headers.

AC_HEADER_STDC
AC_HEADER_STAT
AC_HEADER_TIME
AC_CHECK_HEADERS([stdarg.h])
AC_CHECK_HEADERS([sys/int_types.h sys/time.h])
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h net/nh.h sys/socket.h])
AC_CHECK_HEADERS([fcntl.h dirent.h sys/ioctl.h sys/sockio.h poll.h termios.h])
AC_CHECK_HEADERS([net/if.h net/if_arp.h net/if_dl.h],,,
[[#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
]])
AC_CHECK_HEADERS([readline/readline.h])
AC_CHECK_HEADERS([byteswap.h])
AC_CHECK_HEADERS([sys/mman.h])
AC_CHECK_HEADERS([sys/utsname.h])

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
AC_CHECK_TYPES([off_t, socklen_t],,,[
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

# --------------------------------------------------------------------------
# C/CPP caps

AC_PROG_GCC_TRADITIONAL
AC_C_CONST
AC_C_BIGENDIAN

# --------------------------------------------------------------------------
# macros

AC_CHECK_DECLS([__bswap_32, __bswap_64, htonll, ntohll],,,[
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_BYTESWAP_H
#include <byteswap.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NET_NH_H
#include <net/nh.h>
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
# AIX specific

AC_CHECK_HEADERS([procinfo.h],,,[[ ]])
AC_CHECK_DECLS([getkerninfo])
AC_CHECK_FUNCS([getprocs64],,,[
#if HAVE_PROCINFO_H
#include <procinfo.h>
#endif
])

# --------------------------------------------------------------------------
# library functions

AC_FUNC_ERROR_AT_LINE
#AC_FUNC_MALLOC
#AC_FUNC_REALLOC
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF

AC_CHECK_FUNCS([gettimeofday memmove memset strchr strcspn strdup strerror])
AC_CHECK_FUNCS([strrchr strtol strtoul strtoll strtoull])
AC_CHECK_FUNCS([if_nametoindex daemon mmap strerror_r scandir uname poll])

AC_CHECK_HEADERS([sys/sysctl.h vm/vm_param.h])
AC_CHECK_FUNCS([sysctlbyname sysctlnametomib])

AC_CHECK_FUNCS([tcgetattr tcsetattr cfsetospeed cfsetispeed])

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

#ALK:	CPPFLAGS="$CPPFLAGS -D_THREAD_SAFE" PTHREAD_LIBS="-pthread",
# posix threads
AC_CHECK_HEADER(pthread.h,,AC_MSG_ERROR([*** POSIX thread support not installed - please install first ***]))

PTHREAD_LIBS=error
AC_MSG_CHECKING(for old style FreeBSD -pthread flag)
AC_EGREP_CPP(yes,
	[#if defined(__FreeBSD_cc_version) || defined(__OpenBSD__)
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
	AC_CHECK_FUNC(pthread_attr_init,
		PTHREAD_LIBS="")
fi
# HP-UX
if test "x$PTHREAD_LIBS" = xerror; then
	AC_CHECK_LIB(pthread, __pthread_attr_init_system,
		PTHREAD_LIBS="-lpthread")
fi
if test "x$PTHREAD_LIBS" = xerror; then
	AC_MSG_ERROR(*** Unable to locate working posix thread library ***)
fi
LDFLAGS="$LDFLAGS $PTHREAD_LIBS"
AC_SUBST(PTHREAD_LIBS)

# Extensions to posix threads
AC_CHECK_HEADERS([pthread_np.h],,,[
#include <pthread.h>
])
AC_CHECK_FUNCS([pthread_cond_reltimedwait_np])

# Check for recursive mutexes
AC_CHECK_FUNCS([pthread_mutexattr_settype pthread_mutexattr_setkind_np])
AC_CHECK_DECLS([PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_RECURSIVE_NP, MUTEX_TYPE_COUNTING_FAST],,,[
#include <pthread.h>
#if HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
])

# Check for read/write locks
# On Linux, rwlock functions declared always, but pthread_rwlock_t may need
# additional defines
AC_CHECK_FUNCS([pthread_rwlock_init pthread_rwlock_timedrdlock pthread_rwlock_timedwrlock])
if test "$ac_cv_func_pthread_rwlock_init" = "yes"; then
	AC_CACHE_CHECK([for pthread_rwlock_t], ac_cv_struct_pthread_rw,
		[AC_TRY_COMPILE([#include <sys/types.h>
		#include <pthread.h>],
		[pthread_rwlock_t rwlock=PTHREAD_RWLOCK_INITIALIZER;],
		ac_cv_struct_pthread_rw=yes, [
			AC_TRY_COMPILE([#define _XOPEN_SOURCE 500
			#include <sys/types.h>
			#include <pthread.h>],
			[pthread_rwlock_t rwlock=PTHREAD_RWLOCK_INITIALIZER;],
			ac_cv_struct_pthread_rw=yes, ac_cv_struct_pthread_rw=no)
			if test "$ac_cv_struct_pthread_rw" = "yes"; then
				CPPFLAGS="$CPPFLAGS -D_XOPEN_SOURCE=500"
			fi
			])])
	if test "$ac_cv_struct_pthread_rw" = "yes"; then
		AC_DEFINE(HAVE_PTHREAD_RWLOCK, 1, Define to 1 if you have working pthread read/write locks)
	fi
fi

# --------------------------------------------------------------------------
# check for UNICODE stuff

AC_CHECK_HEADERS([wchar.h iconv.h])
AC_CHECK_SIZEOF(wchar_t)
AC_CHECK_LIB(iconv, iconv, ac_found_iconv=yes)
AC_CHECK_FUNCS(iconv, ac_found_iconv=yes, ac_found_iconv=no)

AC_MSG_CHECKING(whether iconv supports UCS-2-INTERNAL)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","UCS-2-INTERNAL")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_UCS_2_INTERNAL, 1, Define to 1 if iconv supports UCS-2-INTERNAL)
    valid_ucs2_code="UCS-2-INTERNAL"
  ],
  [ AC_MSG_RESULT(no) ],
  [ AC_MSG_RESULT(no) ]
)

AC_MSG_CHECKING(whether iconv supports UCS-2)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","UCS-2")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_UCS_2, 1, Define to 1 if iconv supports UCS-2)
    valid_ucs2_code="UCS-2"
  ],
  [ AC_MSG_RESULT(no) ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_UCS_2, 1, Define to 1 if iconv supports UCS-2)
  ]
)

AC_MSG_CHECKING(whether iconv supports UCS2)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","UCS2")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_UCS2, 1, Define to 1 if iconv supports UCS2)
    valid_ucs2_code="UCS2"
  ],
  [ AC_MSG_RESULT(no) ],
  [ AC_MSG_RESULT(no) ]
)

AC_MSG_CHECKING(whether iconv supports //IGNORE)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8//IGNORE","$valid_ucs2_code")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_IGNORE, 1, Define to 1 if iconv supports //IGNORE option)
  ],
  [ AC_MSG_RESULT(no) ],
  [ AC_MSG_RESULT(no) ]
)

# taken from ZSH's configure
# Check if iconv uses const in prototype declaration
if test "x$ac_found_iconv" = "xyes"; then
  AC_CACHE_CHECK(for iconv declaration, ac_cv_iconv_const,
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>
        #include <iconv.h>]],
        [[#ifdef __cplusplus
          "C"
          #endif
          #if defined(__STDC__) || defined(__cplusplus)
          size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
          #else
          size_t iconv();
          #endif]])],
      [ac_cv_iconv_const=],
      [ac_cv_iconv_const=const])])
  AC_DEFINE_UNQUOTED([ICONV_CONST], $ac_cv_iconv_const,
    [Define as const if the declaration of iconv() needs const.])
fi



if test $ac_cv_sizeof_wchar_t -eq 2; then
	AC_DEFINE(HAVE_USEABLE_WCHAR_T, 1, Define to 1 if you have useable wchar_t)
fi

# --------------------------------------------------------------------------
# variables

AC_SUBST(ALL_STATIC_FLAG)

# misc
CPPFLAGS="$CPPFLAGS -DDATADIR=\\\"\${pkgdatadir}\\\" -DLIBDIR=\\\"\${libdir}\\\""

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

LIBNETXMS_LIBRARY_VERSION=0:2:0
LIBNXCSCP_LIBRARY_VERSION=0:2:0
LIBNXMAP_LIBRARY_VERSION=0:2:0
LIBNXCL_LIBRARY_VERSION=0:2:0
LIBNXSNMP_LIBRARY_VERSION=0:2:0
LIBNXSRV_LIBRARY_VERSION=0:2:0
LIBNXCORE_LIBRARY_VERSION=0:2:0
DBDRV_LIBRARY_VERSION=0:2:0
SMSDRV_LIBRARY_VERSION=0:2:0
LIBNXSL_LIBRARY_VERSION=0:2:0
AC_SUBST(LIBNETXMS_LIBRARY_VERSION)
AC_SUBST(LIBNXCSCP_LIBRARY_VERSION)
AC_SUBST(LIBNXMAP_LIBRARY_VERSION)
AC_SUBST(LIBNXCL_LIBRARY_VERSION)
AC_SUBST(LIBNXSNMP_LIBRARY_VERSION)
AC_SUBST(LIBNXSRV_LIBRARY_VERSION)
AC_SUBST(LIBNXCORE_LIBRARY_VERSION)
AC_SUBST(DBDRV_LIBRARY_VERSION)
AC_SUBST(SMSDRV_LIBRARY_VERSION)
AC_SUBST(LIBNXSL_LIBRARY_VERSION)

# --------------------------------------------------------------------------

AC_CONFIG_FILES([
   README
	Makefile
	contrib/Makefile
	contrib/dct/Makefile
	contrib/startup/Makefile
	contrib/startup/gentoo/Makefile
	contrib/startup/redhat/Makefile
	contrib/mibs/Makefile
	contrib/backgrounds/Makefile
	doc/Makefile
	doc/internal/Makefile
	doc/manuals/Makefile
	doc/misc/Makefile
	images/Makefile
	include/Makefile
	m4/Makefile
	netware/Makefile
	sql/Makefile
	src/Makefile
	src/libnetxms/Makefile
	src/libnxcscp/Makefile
	src/libnxsnmp/Makefile
	src/libnxcl/Makefile
	src/libnxsl/Makefile
	src/libnxmap/Makefile
	src/install/Makefile
	src/install/windows/Makefile
	src/console/Makefile
	src/console/nxuilib/Makefile
	src/console/nxuilib/res/Makefile
	src/console/nxuilib/sounds/Makefile
	src/console/nxlexer/Makefile
	src/console/win32/Makefile
	src/console/win32/icons/Makefile
	src/console/win32/res/Makefile
	src/console/win32starter/Makefile
	src/console/nxav/Makefile
	src/console/nxav/res/Makefile
	src/console/nxnotify/Makefile
	src/console/nxnotify/res/Makefile
	src/console/cmdline/Makefile
	src/console/PocketPC/Makefile
	src/console/PocketPC/res/Makefile
	src/nxevent/Makefile
	src/nxreport/Makefile
	src/nxmibc/Makefile
	src/nxscript/Makefile
	src/server/Makefile
	src/server/core/Makefile
	src/server/netxmsd/Makefile
	src/server/dbdrv/Makefile
	src/server/dbdrv/odbc/Makefile
	src/server/dbdrv/mssql/Makefile
	src/server/dbdrv/mysql/Makefile
	src/server/dbdrv/pgsql/Makefile
	src/server/dbdrv/sqlite/Makefile
	src/server/smsdrv/Makefile
	src/server/smsdrv/generic/Makefile
	src/server/tools/Makefile
	src/server/tools/nxget/Makefile
	src/server/tools/nxaction/Makefile
	src/server/tools/nxadm/Makefile
	src/server/tools/nxconfig/Makefile
	src/server/tools/nxconfig/res/Makefile
	src/server/tools/nxdbmgr/Makefile
	src/server/tools/nxsnmpget/Makefile
	src/server/tools/nxsnmpwalk/Makefile
	src/server/tools/nxsnmpset/Makefile
	src/server/tools/nxupload/Makefile
	src/server/libnxsrv/Makefile
	src/agent/Makefile
	src/agent/core/Makefile
	src/agent/install/Makefile
	src/agent/subagents/Makefile
	src/agent/subagents/linux/Makefile
	src/agent/subagents/sunos/Makefile
	src/agent/subagents/aix/Makefile
	src/agent/subagents/hpux/Makefile
	src/agent/subagents/skeleton/Makefile
	src/agent/subagents/freebsd/Makefile
	src/agent/subagents/openbsd/Makefile
	src/agent/subagents/ipso/Makefile
	src/agent/subagents/netware/Makefile
	src/agent/subagents/winnt/Makefile
	src/agent/subagents/win9x/Makefile
	src/agent/subagents/winperf/Makefile
	src/agent/subagents/portCheck/Makefile
	src/agent/subagents/logscan/Makefile
	src/agent/subagents/ping/Makefile
	src/agent/subagents/ups/Makefile
	src/nxhttpd/Makefile
	src/nxhttpd/static/Makefile
	src/nxhttpd/static/images/Makefile
	tools/Makefile
])

AC_OUTPUT

#echo "Updating libtool-1"
#sed 's/-lc_r/-lc_rXXX/' < ./libtool > libtool.new && mv libtool.new libtool && chmod +x ./libtool

echo "Updating libtool-2"
sed 's,$SED -e "/${host}-//g",$SED -e "s/${host}-//g",g' < ./libtool > libtool.new && mv libtool.new libtool && chmod +x ./libtool

#if test ! "x$BUILD_SERVER" = "xyes" ; then
#   echo "Updating libtool-3"
#   $PERL tools/patch_libtool.pl <libtool >libtool.new
#   mv libtool.new libtool
#   chmod 755 libtool
#fi



###########################
#
echo
echo
echo
echo '---------------------------------------------------------------------'
echo '                         Configure results'
echo '---------------------------------------------------------------------'
echo
	echo "Prefix              : ${prefix}"
if test "x${BUILD_SERVER}" = "xyes"; then
	echo "Build Server        : YES"
else
	echo "Build Server        : NO"
fi
if test "x${ac_cv_lib_crypto_RSA_new}" = "xyes"; then
	echo "Encryption enabled  : YES"
else
	echo "Encryption enabled  : NO"
fi
if test "x${DB_DRIVERS}" != "x"; then
	echo "Build DB-Drivers    : ${DB_DRIVERS}"
else
	echo "Build DB-Drivers    : NO"
fi
if test "x${BUILD_CLIENT}" = "xyes"; then
	echo "Build Clients       : YES"
else
	echo "Build Clients       : NO"
fi
if test "x${BUILD_WEB_CLIENT}" = "xyes"; then
	echo "Build Web Interface : YES"
else
	echo "Build Web Interface : NO"
fi
if test "x${BUILD_AGENT}" = "xyes"; then
	echo "Build Agent         : YES"
	echo "Subagents list      : ${SUBAGENT_DIRS}"
else
	echo "Build Agent         : NO"
fi
if test "x${BUILD_STATIC_AGENT}" = "xyes"; then
	echo "Build Static Agent  : YES"
	echo "Subagents list      : ${STATIC_SUBAGENT_LIST}"
else
	echo "Build Static Agent  : NO"
fi
