# $Id$
#
# NetXMS - Network Management System
# Configure script
#

AC_INIT([NetXMS], [0.9.0-rc2], [NetXMS Team <bugs@netxms.org>])
AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE


#--------------------------------------------------------------------
# Functions
#--------------------------------------------------------------------

check_substr()
{
	for w in $1; do
		if test "x$w" = "x$2"; then
			return 0;
		fi
	done
	return 1
}


#--------------------------------------------------------------------
# Initialize variables
#--------------------------------------------------------------------

PLATFORM=`uname -s`
COMPONENTS=""
DISABLE_ICONV="no"
DISABLE_ENCRYPTION="no"
ENABLE_UNSTABLE="no"
ENABLE_DEBUG="no"
STATIC_BUILD="no"
DBDRIVER_SELECTED="no"
USE_PTH="no"
BUILD_SERVER="no"
BUILD_AGENT="no"
BUILD_CLIENT="no"
BUILD_CONSOLE="no"
BUILD_NXHTTPD="no"
BUILD_STATIC_AGENT="no"
NEED_ZLIB="no"
NEED_WXWIDGETS="no"
MODULES=""
STATIC_SUBAGENT_LIST=""
SUBAGENT_DIRS=""
SUBAGENT_LIBS=""
UNSTABLE_SUBAGENT_DIRS=""
SERVER_TOOLS=""
TOP_LEVEL_MODULES=""
CONTRIB_MODULES=""
CLIENT_COMPONENTS=""
BUILD_UNICODE="no"
WX_UNICODE="no"


#--------------------------------------------------------------------
# Parse command line parameters
#--------------------------------------------------------------------

AC_ARG_WITH(server,
[AS_HELP_STRING(--with-server,build server)],
[
	COMPONENTS="$COMPONENTS snmp server agent"
])

AC_ARG_WITH(snmp,
[AS_HELP_STRING(--with-snmp,build SNMP library and tools)],
[
	COMPONENTS="$COMPONENTS snmp"
])

AC_ARG_WITH(client,
[AS_HELP_STRING(--with-client,build client library and tools)],
[
	COMPONENTS="$COMPONENTS client"
])

AC_ARG_WITH(console,
[AS_HELP_STRING(--with-console,build management console)],
[
	COMPONENTS="$COMPONENTS snmp console client"
])

AC_ARG_WITH(agent,
[AS_HELP_STRING(--with-agent,build agent)],
[
	COMPONENTS="$COMPONENTS agent"
])

AC_ARG_WITH(static-agent,
[AS_HELP_STRING(--with-static-agent,build statically linked agent)],
[
	COMPONENTS="$COMPONENTS static-agent"
])

AC_ARG_WITH(ipso-agent,
[AS_HELP_STRING(--with-ipso-agent,build statically linked IPSO agent)],
[
	COMPONENTS="$COMPONENTS ipso-agent"
])

AC_ARG_WITH(nxhttpd,
[AS_HELP_STRING(--with-nxhttpd,build web interface)],
[
	COMPONENTS="$COMPONENTS nxhttpd"
])

AC_ARG_WITH(mdebug,
[AS_HELP_STRING(--with-mdebug,use debug version of memory allocation functions)],
[
	CPPFLAGS="$CPPFLAGS -DNETXMS_MEMORY_DEBUG"
])

AC_ARG_WITH(private,
[AS_HELP_STRING(--with-private,include private sources into build)],
[
	COMPONENTS="$COMPONENTS private"
])

AC_ARG_WITH(sqlite,
[AS_HELP_STRING(--with-sqlite,build SQLite database driver)],
[ if test "x$withval" != "xno" ; then
	COMPONENTS="$COMPONENTS sqlite"
	DBDRIVER_SELECTED="yes"
fi ])

AC_ARG_WITH(odbc,
[AS_HELP_STRING(--with-odbc,build ODBC database driver)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
		LDFLAGS="$LDFLAGS -L${withval}/lib"
		CPPFLAGS="$CPPFLAGS -I${withval}/include"
	fi
	COMPONENTS="$COMPONENTS odbc"
	DBDRIVER_SELECTED="yes"
fi ])

AC_ARG_WITH(oracle,
[AS_HELP_STRING(--with-oracle,build Oracle database driver)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
		LDFLAGS="$LDFLAGS -L${withval}/lib"
		CPPFLAGS="$CPPFLAGS -I${withval}/include"
	fi
	COMPONENTS="$COMPONENTS oracle"
	DBDRIVER_SELECTED="yes"
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
	COMPONENTS="$COMPONENTS mysql"
	DBDRIVER_SELECTED="yes"
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
	COMPONENTS="$COMPONENTS pgsql"
	DBDRIVER_SELECTED="yes"
fi ])

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
	fi
	REQUIRE_ENCRYPTION="yes"
])

AC_ARG_WITH(gd,
[AS_HELP_STRING(--with-gd,specify GD location)],
[
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"    
		LDFLAGS="$LDFLAGS -L${withval}/lib"
		CPPFLAGS="$CPPFLAGS -I${withval}/include"
	fi
])

AC_ARG_ENABLE(iconv,
[AS_HELP_STRING(--disable-iconv,do not use iconv() for text conversions)],
[
	DISABLE_ICONV="yes"
])

AC_ARG_ENABLE(encryption,
[AS_HELP_STRING(--disable-encryption,disable encryption support)],
[
	DISABLE_ENCRYPTION="yes"
])

AC_ARG_ENABLE(unstable,
[AS_HELP_STRING(--enable-unstable,build experimental/unstable components)],
[
	ENABLE_UNSTABLE="yes"
])

AC_ARG_ENABLE(debug,
[AS_HELP_STRING(--enable-debug,enable additional debugging functionality)],
[
	ENABLE_DEBUG="yes"
])

AC_ARG_WITH(dist,
[AS_HELP_STRING(--with-dist,for maintainers only)],
	DB_DRIVERS=" mysql pgsql odbc mssql sqlite oracle"
	MODULES="snmp libnxsl libnxlp server agent libnxmap libnxcl client console nxhttpd nxscript nxcptest"
	SUBAGENT_DIRS="linux freebsd openbsd netbsd sunos aix ipso hpux"
	NXCONFIG="nxconfig"
	TOP_LEVEL_MODULES="sql images"
	CONTRIB_MODULES="mibs backgrounds"
	CLIENT_COMPONENTS="console nxalarm nxevent nxpush nxsms"
	UNSTABLE_SUBAGENT_DIRS="odbcquery"
	SERVER_TOOLS="nxconfig"
	NEED_WXWIDGETS="yes"
)


#--------------------------------------------------------------------
# Validate command line parameters and set various checking options
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "static-agent"
if test $? = 0; then
	if test "x$COMPONENTS" != "x static-agent"; then
		AC_MSG_ERROR(Static agent and other components are mutally exclusive.)
	fi
	STATIC_BUILD="yes"
	BUILD_STATIC_AGENT="yes"
	MODULES="$MODULES libnxlp agent"
	DISABLE_ICONV="yes"
	CPPFLAGS="$CPPFLAGS -D_STATIC_AGENT"

	case "$PLATFORM" in
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
		NetBSD)
			SUBAGENT_DIRS="netbsd"
			SUBAGENT_LIBS="../subagents/netbsd/libnsm_netbsd.la"
			STATIC_SUBAGENT_LIST="netbsd $STATIC_SUBAGENT_LIST"
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
fi

check_substr "$COMPONENTS" "ipso-agent"
if test $? = 0; then
	if test "x$COMPONENTS" != "x ipso-agent"; then
		AC_MSG_ERROR(IPSO agent and other components are mutally exclusive.)
	fi
	STATIC_BUILD="yes"
	BUILD_STATIC_AGENT="yes"
	DISABLE_ICONV="yes"
	USE_PTH="yes"
	MODULES="$MODULES libnxlp agent"
	CPPFLAGS="$CPPFLAGS -D_STATIC_AGENT -D_IPSO -D_USE_GNU_PTH -I/usr/local/include"
	LDFLAGS="$LDFLAGS -all-static"
	STATIC_SUBAGENT_LIST="ipso ping portcheck ups"
	SUBAGENT_DIRS="ipso"
	SUBAGENT_LIBS="../subagents/ipso/libnsm_ipso.la ../subagents/ping/libnsm_ping.la ../subagents/portCheck/libnsm_portCheck.la ../subagents/ups/libnsm_ups.la"
fi

check_substr "$COMPONENTS" "snmp"
if test $? = 0; then
	NEED_ZLIB="yes"
	MODULES="$MODULES snmp"
fi

check_substr "$COMPONENTS" "server"
if test $? = 0; then
	if test "x$DBDRIVER_SELECTED" != "xyes"; then
		AC_MSG_ERROR(You must select at least one database driver when building server.)
	fi
	BUILD_SERVER="yes"
	MODULES="$MODULES libnxmap libnxsl libnxlp server nxscript"
	TOP_LEVEL_MODULES="$TOP_LEVEL_MODULES sql images"
	CONTRIB_MODULES="$CONTRIB_MODULES mibs backgrounds"
fi

check_substr "$COMPONENTS" "console"
if test $? = 0; then
	BUILD_CONSOLE="yes"
	NEED_WXWIDGETS="yes"
	CLIENT_COMPONENTS="$CLIENT_COMPONENTS console"
fi

check_substr "$COMPONENTS" "client"
if test $? = 0; then
	BUILD_CLIENT="yes"
	NEED_ZLIB="yes"
	MODULES="$MODULES libnxmap libnxcl console client"
	CLIENT_COMPONENTS="$CLIENT_COMPONENTS nxalarm nxevent nxpush nxsms"
fi

check_substr "$COMPONENTS" "agent"
if test $? = 0; then
	BUILD_AGENT="yes"
	MODULES="$MODULES libnxlp agent"
	if test "x$ENABLE_UNSTABLE" = "xyes"; then
		UNSTABLE_SUBAGENT_DIRS="odbcquery"
	fi

	case "$PLATFORM" in
		Linux)
			SUBAGENT_DIRS="linux"
			;;
		FreeBSD)
			SUBAGENT_DIRS="freebsd"
			;;
		OpenBSD)
			SUBAGENT_DIRS="openbsd"
			;;
		NetBSD)
			SUBAGENT_DIRS="netbsd"
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
fi

check_substr "$COMPONENTS" "nxhttpd"
if test $? = 0; then
	BUILD_NXHTTPD="yes"
	MODULES="$MODULES libnxmap libnxcl nxhttpd"
fi

check_substr "$COMPONENTS" "mysql"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS mysql"
fi

check_substr "$COMPONENTS" "pgsql"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS pgsql"
fi

check_substr "$COMPONENTS" "sqlite"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS sqlite"
fi

check_substr "$COMPONENTS" "odbc"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS odbc"
fi

check_substr "$COMPONENTS" "oracle"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS oracle"
fi

check_substr "$COMPONENTS" "private"
if test $? = 0; then
	MODULES="$MODULES private"
fi


#--------------------------------------------------------------------
# Check for programs
#--------------------------------------------------------------------

if test "x$PLATFORM" = "xHP-UX"; then
	AC_CHECK_PROG([CCC], [aCC], [aCC])
fi
AC_PROG_CC(aCC gcc cc cl)
AC_PROG_CPP
AC_PROG_CXX
if test "x$CXX" = "xg++"; then
	AC_CHECK_PROG([GPLUSPLUS], [g++], [yes], [no])
	if test "x$GPLUSPLUS" = "xno"; then
		AC_MSG_ERROR([*** FATAL: Cannot find any usable C++ compiler])
	fi
fi
AC_PROG_INSTALL
AC_PROG_LEX
AC_CHECK_PROGS(YACC,bison byacc,yacc)
AC_PROG_LN_S

AC_ARG_VAR(PERL,local path to the perl interpreter)
perl_possible_path="/usr/bin:/usr/local/bin:/bin:/opt/perl/bin:/opt/perl/usr/bin:/opt/perl/usr/local/bin"
AC_PATH_PROG(PERL,perl,/usr/bin/env perl,$perl_possible_path)


#--------------------------------------------------------------------
# Initialize libtool
#--------------------------------------------------------------------

if test "x$STATIC_BUILD" = "xyes"; then
	enable_shared=no
	enable_static=yes
else
	enable_shared=yes
	enable_static=no
fi
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL


#--------------------------------------------------------------------
# C/C++ capabilities
#--------------------------------------------------------------------

AC_C_CONST
AC_PROG_GCC_TRADITIONAL
AC_C_BIGENDIAN


#--------------------------------------------------------------------
# Platform-dependent settings
#--------------------------------------------------------------------

case "$PLATFORM" in
   AIX)
      LDFLAGS="-no-undefined $LDFLAGS"
      ;;
   HP-UX)
      CPPFLAGS="$CPPFLAGS -D_HPUX -D_HPUX_SOURCE -D_POSIX_PTHREAD_SEMANTICS -D_XOPEN_SOURCE=500 -D_XOPEN_SOURCE_EXTENDED=1 -D_REENTRANT"
      if test "x$CXX" = "xaCC"; then
         CXXFLAGS="-mt $CXXFLAGS +W749 +W829"
         LDFLAGS="$LDFLAGS -mt"
      fi
      ;;
   SunOS)
      LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
      export LD_LIBRARY_PATH
      ;;
   Linux)
      CPPFLAGS="$CPPFLAGS -D_GNU_SOURCE"
      ;;
   *)
      ;;
esac


#--------------------------------------------------------------------
# C++ compiler tuning:
#  * Disable C++ exceptions
#  * Disable compiling string constants as "const char *"
#  * Disable some GCC warnings
#  * Turn on 64bit mode on HP-UX/Itanium
#--------------------------------------------------------------------

if test "x$CC" = "xgcc" ; then

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

	AC_MSG_CHECKING(whether C++ compiler accepts -fno-const-strings)
	OLD_CXXFLGAS="$CXXFLAGS"
	CXXFLAGS="$CXXFLAGS -fno-const-strings"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
		[ AC_MSG_RESULT(yes) ], 
		[ 
			CXXFLAGS="$OLD_CXXFLAGS"
			AC_MSG_RESULT(no)
		])

	AC_MSG_CHECKING(whether C++ compiler accepts -Wno-deprecated)
	OLD_CXXFLGAS="$CXXFLAGS"
	CXXFLAGS="$CXXFLAGS -Wno-deprecated"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
		[ AC_MSG_RESULT(yes) ], 
		[ 
			CXXFLAGS="$OLD_CXXFLAGS"
			AC_MSG_RESULT(no)
		])

	AC_LANG_POP([C++])

	if test "x$PLATFORM" = "xHP-UX"; then
		AC_MSG_CHECKING(whether C compiler accepts -mlp64)
		OLD_CPPFLGAS="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS -mlp64"
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
			[ 
				LDFLAGS="-mlp64 -L/usr/lib/hpux64 -L/usr/local/lib/hpux64 $LDFLAGS"
				AC_MSG_RESULT(yes) 
			], 
			[ 
				CPPFLAGS="$OLD_CPPFLAGS"
				AC_MSG_RESULT(no)
			])
	fi

	if test "x$ENABLE_DEBUG" = "xyes"; then
		CPPFLAGS="$CPPFLAGS -ggdb3"
	fi
fi


#--------------------------------------------------------------------
# Additional include and library directories
#--------------------------------------------------------------------

if test -d /usr/local/include; then
	CPPFLAGS="$CPPFLAGS -I/usr/local/include"
fi

if test -d /usr/kerberos/include; then
	CPPFLAGS="$CPPFLAGS -I/usr/kerberos/include"
fi

if test -d /usr/local/lib; then
	LDFLAGS="$LDFLAGS -L/usr/local/lib"
fi


#--------------------------------------------------------------------
# Check for threads library - either POSIX or GNU
#--------------------------------------------------------------------

if test "x$USE_PTH" = "xyes"; then
	AC_CHECK_HEADER(pth.h,,AC_MSG_ERROR([*** GNU Pth thread support not installed - please install first ***]))
else
	AC_CHECK_HEADER(pthread.h,,AC_MSG_ERROR([*** POSIX thread support not installed - please install first ***]))

	if test "x$CXX" = "xaCC"; then
		PTHREAD_LIBS=""
	else
		PTHREAD_LIBS="error"
		AC_MSG_CHECKING(for old style FreeBSD -pthread flag)
		AC_EGREP_CPP(yes,
			[#if defined(__FreeBSD_cc_version) || defined(__OpenBSD__)
				yes
			#endif
			], AC_MSG_RESULT(yes)
			CPPFLAGS="$CPPFLAGS -D_THREAD_SAFE" PTHREAD_LIBS="-pthread",
			AC_MSG_RESULT(no))
		if test "x$PTHREAD_LIBS" = "xerror"; then
			AC_CHECK_LIB(pthread, pthread_attr_init,
				PTHREAD_LIBS="-lpthread")
		fi
		if test "x$PTHREAD_LIBS" = "xerror"; then
			AC_CHECK_LIB(pthreads, pthread_attr_init,
				PTHREAD_LIBS="-lpthreads")
		fi
		if test "x$PTHREAD_LIBS" = "xerror"; then
			AC_CHECK_FUNC(pthread_attr_init,
				PTHREAD_LIBS="")
		fi
	fi

	if test "x$PTHREAD_LIBS" = "xerror"; then
		AC_MSG_ERROR(*** Unable to locate working posix thread library ***)
	fi
	LDFLAGS="$LDFLAGS $PTHREAD_LIBS"

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
			[AC_TRY_COMPILE([
#include <sys/types.h>
#include <pthread.h>
			],
			[pthread_rwlock_t rwlock=PTHREAD_RWLOCK_INITIALIZER;],
			ac_cv_struct_pthread_rw=yes, [
				AC_TRY_COMPILE([
#define _XOPEN_SOURCE 500
#include <sys/types.h>
#include <pthread.h>
				],
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

fi


#--------------------------------------------------------------------
# Checks for OpenSSL
#--------------------------------------------------------------------

if test "x$DISABLE_ENCRYPTION" != "xyes" ; then
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
		[
			if test "x$REQUIRE_ENCRYPTION" = "xyes"; then
				AC_MSG_ERROR(libcrypto is required for encryption support)
			else
				AC_MSG_RESULT(libcrypto is required for encryption support)
			fi
		])
fi


#--------------------------------------------------------------------
# Checks for other libs.
#--------------------------------------------------------------------

AC_CHECK_LIB(xnet, accept)
AC_CHECK_LIB(socket, if_nameindex)
AC_CHECK_LIB(dl, dlopen)
AC_CHECK_LIB(kstat, kstat_open)
AC_CHECK_LIB(gnugetopt, getopt_long)

if test "x$BUILD_SERVER" = "xyes" ; then
   AC_CHECK_LIB(termcap, tgetstr, [], [AC_CHECK_LIB(ncurses, tgetstr, [], AC_CHECK_LIB(curses, tgetstr))])
   AC_CHECK_LIB(readline, readline)
fi

if test "x$NEED_ZLIB" = "xyes"; then
	AC_CHECK_HEADER(zlib.h,,AC_MSG_ERROR([*** ZLib development package not installed - please install first ***]))
	AC_CHECK_LIB(z, deflate)
fi

HAVE_LIBEXPAT=yes
AC_CHECK_HEADER(expat.h,,HAVE_LIBEXPAT=no)
AC_CHECK_LIB(expat, XML_Parse, [], [ HAVE_LIBEXPAT=no ])
if test "x$HAVE_LIBEXPAT" = "xyes"; then
	AC_DEFINE(HAVE_LIBEXPAT,1,Define to 1 if you have working libexpat)
	MODULES="$MODULES nxcptest"
fi

#--------------------------------------------------------------------
# Checks for header files
#--------------------------------------------------------------------

AC_CHECK_HEADERS([sys/types.h sys/stat.h unistd.h stdarg.h fcntl.h alloca.h])
AC_CHECK_HEADERS([sys/int_types.h time.h sys/time.h sys/utsname.h])
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h net/nh.h sys/socket.h])
AC_CHECK_HEADERS([fcntl.h dirent.h sys/ioctl.h sys/sockio.h poll.h termios.h])
AC_CHECK_HEADERS([readline/readline.h byteswap.h sys/select.h])
AC_CHECK_HEADERS([sys/sysctl.h vm/vm_param.h syslog.h getopt.h])
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


#--------------------------------------------------------------------
# Checks for data types
#--------------------------------------------------------------------

AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)

AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_STRUCT_TIMEZONE

AC_CHECK_TYPES([long long, unsigned long long, int64_t, uint64_t, u_int64_t])
AC_CHECK_TYPES([mode_t, off_t, socklen_t],,,[
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])
AC_CHECK_TYPES([struct stat64],,,[
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
])


#--------------------------------------------------------------------
# Checks for functions
#--------------------------------------------------------------------

AC_FUNC_ERROR_AT_LINE
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF

AC_CHECK_FUNCS([gettimeofday memmove memset strchr strcspn strdup strerror])
AC_CHECK_FUNCS([strrchr strtol strtoul strtoll strtoull])
AC_CHECK_FUNCS([if_nametoindex daemon mmap strerror_r scandir uname poll])
AC_CHECK_FUNCS([nanosleep getopt_long gmtime_r localtime_r lstat64])

# Check if timezone is a function
AC_MSG_CHECKING(for timezone function)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
]],[[
int main()
{
   timezone(0, 0);
   return 0;
}
]])
],[
AC_MSG_RESULT(yes)
AC_DEFINE(HAVE_TIMEZONE,1,Define to 1 if timezone function is available)
],[
AC_MSG_RESULT(no)
AC_CHECK_DECLS([timezone],,,[
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
])
])

# Check if struct tm has tm_gmtoff member
AC_MSG_CHECKING(for struct tm.tm_gmtoff)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
]],[[
int main()
{
   struct tm t;
   return t.tm_gmtoff;
}
]])
],[
AC_MSG_RESULT(yes)
AC_DEFINE(HAVE_TM_GMTOFF,1,Define to 1 if struct tm has tm_gmtoff member)
],[
AC_MSG_RESULT(no)
])

AC_CHECK_DECLS([getopt_long],,,[
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
])

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


#--------------------------------------------------------------------
# Checks for macros and definitions
#--------------------------------------------------------------------

AC_CHECK_DECLS([__bswap_32, __bswap_64, htonll, ntohll],,,[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
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


#--------------------------------------------------------------------
# AIX specific checks
#--------------------------------------------------------------------

if test "x$PLATFORM" = "xAIX"; then
	AC_CHECK_HEADERS([procinfo.h],,,[[ ]])
	AC_CHECK_DECLS([getkerninfo])
	AC_CHECK_FUNCS([getprocs64],,,[
#if HAVE_PROCINFO_H
#include <procinfo.h>
#endif
	])
fi


#--------------------------------------------------------------------
# Check for wxWidgets
#--------------------------------------------------------------------

if test "x$NEED_WXWIDGETS" = "xyes"; then
	AC_PATH_PROGS([WXCONF], [wx-config wxgtk2-2.8-config], [no], [$PATH:/usr/local/bin])
	if test "x$WXCONF" = "xno"; then
		AC_MSG_ERROR([*** wxWidgets not installed - unable to build GUI components ***])
	else
		AC_PATH_PROGS([WXRC], [wxrc wxrc-gtk2-2.8], [no], [$PATH:/usr/local/bin])
		if test "x$WXRC" = "xno"; then
			AC_MSG_ERROR([*** wxWidgets resource compiler not found - unable to build GUI components ***])
		else
			NEED_UNICODE=`$WXCONF --selected-config | grep unicode | wc -l`
			if test $NEED_UNICODE -eq 1; then
				BUILD_UNICODE="yes"
				WX_UNICODE="yes"
			fi
			CXXFLAGS="$CXXFLAGS `$WXCONF --cxxflags --debug=$ENABLE_DEBUG`"
			LDFLAGS="$LDFLAGS `$WXCONF --libs --debug=$ENABLE_DEBUG`"
		fi
	fi
	$WXCONF --cxxflags | grep error >/dev/null
	if test $? = 0; then
		echo "wx-config output:"
		echo "*** BEGIN ***"
		$WXCONF --cxxflags
		echo "*** END ***"
		AC_MSG_ERROR([*** wxWidgets is not configured properly - see wx-config output above ***])
	fi
fi


#--------------------------------------------------------------------
# Check for UNICODE stuff
#--------------------------------------------------------------------

AC_CHECK_HEADERS([wchar.h wctype.h iconv.h])
AC_CHECK_TYPES([wchar_t],,,[
#if HAVE_WCHAR_H
#include <wchar.h>
#endif
])
AC_CHECK_SIZEOF(wchar_t)
AC_CHECK_FUNCS([wcslen wcsdup wcsncpy wcstoll wcstoull wfopen wopen wstat wgetenv towupper wcserror wcserror_r])

if test "x$DISABLE_ICONV" != "xyes"; then
	AC_CHECK_LIB(iconv, libiconv_open,
		[ ac_found_iconv=yes
		  LDFLAGS="$LDFLAGS -liconv"
		  AC_CHECK_FUNCS(libiconv, ac_found_iconv=yes, ac_found_iconv=no)
		],
		[
			AC_CHECK_LIB(iconv, iconv, 
				[ ac_found_iconv=yes
			          LDFLAGS="$LDFLAGS -liconv"
			])
			AC_CHECK_FUNCS(iconv, ac_found_iconv=yes, ac_found_iconv=no)
		])
else
	ac_found_iconv=no
	CPPFLAGS="$CPPFLAGS -D__DISABLE_ICONV"
fi

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
  [ AC_MSG_RESULT(no) ]
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

AC_MSG_CHECKING(whether iconv supports UCS-2BE)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","UCS-2BE")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_UCS_2BE, 1, Define to 1 if iconv supports UCS-2BE)
    valid_ucs2_code="UCS-2BE"
  ],
  [ AC_MSG_RESULT(no) ],
  [ AC_MSG_RESULT(no) ]
)

AC_MSG_CHECKING(whether iconv supports UCS-2LE)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","UCS-2LE")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_UCS_2LE, 1, Define to 1 if iconv supports UCS-2LE)
    valid_ucs2_code="UCS-2LE"
  ],
  [ AC_MSG_RESULT(no) ],
  [ AC_MSG_RESULT(no) ]
)

AC_MSG_CHECKING(whether iconv supports UCS-4-INTERNAL)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","UCS-4-INTERNAL")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_UCS_4_INTERNAL, 1, Define to 1 if iconv supports UCS-4-INTERNAL)
    valid_ucs4_code="UCS-4-INTERNAL"
  ],
  [ AC_MSG_RESULT(no) ],
  [ AC_MSG_RESULT(no) ]
)

AC_MSG_CHECKING(whether iconv supports UCS-4)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","UCS-4")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_UCS_4, 1, Define to 1 if iconv supports UCS-4)
    valid_ucs4_code="UCS-4"
  ],
  [ AC_MSG_RESULT(no) ],
  [ AC_MSG_RESULT(no) ]
)

AC_MSG_CHECKING(whether iconv supports UCS4)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","UCS4")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_UCS4, 1, Define to 1 if iconv supports UCS4)
    valid_ucs4_code="UCS4"
  ],
  [ AC_MSG_RESULT(no) ],
  [ AC_MSG_RESULT(no) ]
)

AC_MSG_CHECKING(whether iconv supports UCS-4BE)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","UCS-4BE")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_UCS_4BE, 1, Define to 1 if iconv supports UCS-4BE)
    valid_ucs4_code="UCS-4BE"
  ],
  [ AC_MSG_RESULT(no) ],
  [ AC_MSG_RESULT(no) ]
)

AC_MSG_CHECKING(whether iconv supports UCS-4LE)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","UCS-4LE")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_UCS_4LE, 1, Define to 1 if iconv supports UCS-4LE)
    valid_ucs4_code="UCS-4LE"
  ],
  [ AC_MSG_RESULT(no) ],
  [ AC_MSG_RESULT(no) ]
)

AC_MSG_CHECKING(whether iconv supports ISO8859-1)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","ISO8859-1")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_ISO8859_1, 1, Define to 1 if iconv supports ISO8859-1)
  ],
  [ AC_MSG_RESULT(no) ],
  [ AC_MSG_RESULT(no) ]
)

AC_MSG_CHECKING(whether iconv supports ISO-8859-1)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","ISO-8859-1")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_ISO_8859_1, 1, Define to 1 if iconv supports ISO-8859-1)
  ],
  [ AC_MSG_RESULT(no) ],
  [ AC_MSG_RESULT(no) ]
)

AC_MSG_CHECKING(whether iconv supports ASCII)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","ASCII")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_ASCII, 1, Define to 1 if iconv supports ASCII)
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
	AC_DEFINE(UNICODE_UCS2, 1, Define to 1 if you have 2-byte wchar_t)
fi

if test $ac_cv_sizeof_wchar_t -eq 4; then
	AC_DEFINE(UNICODE_UCS4, 1, Define to 1 if you have 4-byte wchar_t)
fi


#--------------------------------------------------------------------
# MySQL
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "mysql"
if test $? = 0; then
	AC_CHECK_LIB(mysqlclient, mysql_init, [],
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
fi


#--------------------------------------------------------------------
# PostgreSQL
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "pgsql"
if test $? = 0; then
	AC_CHECK_LIB(m, floor)
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
	AC_CHECK_LIB(pq, PQfformat, [ AC_DEFINE(HAVE_PQFFORMAT, 1, PQfformat not exist in 7.3) ])
fi


#--------------------------------------------------------------------
# ODBC
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "odbc"
if test $? = 0; then
	AC_CHECK_LIB(odbc, SQLAllocHandle, [],
		[AC_MSG_ERROR(libodbc is required for ODBC support)])
fi


#--------------------------------------------------------------------
# Oracle
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "oracle"
if test $? = 0; then
	AC_CHECK_HEADER(oci.h,,AC_MSG_ERROR([*** Cannot find oci.h - check your Oracle client installation ***]))
	AC_CHECK_LIB(clntsh, OCIHandleAlloc, [],
		[AC_MSG_ERROR(libclntsh is required for Oracle support)])
fi


#--------------------------------------------------------------------
# nxhttpd
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "nxhttpd"
if test $? = 0; then
	AC_CHECK_HEADER(gd.h,,AC_MSG_ERROR([*** GD support not installed - please install first ***]))
	AC_CHECK_LIB(gd, gdImageCreate, [], [
			AC_MSG_ERROR(libgd is required for web interface)
		])

fi


#--------------------------------------------------------------------
# Other settings
#--------------------------------------------------------------------

CPPFLAGS="$CPPFLAGS -D_THREAD_SAFE -DPREFIX=\\\"\${prefix}\\\" -DPREFIXW=L\\\"\${prefix}\\\" -DDATADIR=\\\"\${pkgdatadir}\\\" -DLIBDIR=\\\"\${libdir}\\\""

AM_CONDITIONAL([BUILD_UNICODE_LIBS], [test "x$BUILD_UNICODE" = "xyes"])
AM_CONDITIONAL([BUILD_UNICODE_CONSOLE], [test "x$WX_UNICODE" = "xyes"])


#--------------------------------------------------------------------
# Substitute variables
#--------------------------------------------------------------------

AC_SUBST(DB_DRIVERS)
AC_SUBST(MODULES)
AC_SUBST(SUBAGENT_DIRS)
AC_SUBST(SERVER_TOOLS)
AC_SUBST(TOP_LEVEL_MODULES)
AC_SUBST(CONTRIB_MODULES)
AC_SUBST(CLIENT_COMPONENTS)
AC_SUBST(STATIC_SUBAGENT_LIST)
AC_SUBST(SUBAGENT_LIBS)
AC_SUBST(BUILD_SERVER)
AC_SUBST(UNSTABLE_SUBAGENT_DIRS)
AC_SUBST(WXRC)

#--------------------------------------------------------------------
# Shared libs versions
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
#--------------------------------------------------------------------

NETXMS_LIBRARY_VERSION=0:2:0
AC_SUBST(NETXMS_LIBRARY_VERSION)


#--------------------------------------------------------------------
# Generate output files
#--------------------------------------------------------------------

AC_CONFIG_FILES([
	README
	Makefile
	contrib/Makefile
	contrib/startup/Makefile
	contrib/startup/gentoo/Makefile
	contrib/startup/redhat/Makefile
	contrib/startup/suse/Makefile
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
	src/libnxcl/Makefile
	src/libnxlp/Makefile
	src/libnxsl/Makefile
	src/libnxmap/Makefile
	src/install/Makefile
	src/install/windows/Makefile
	src/client/Makefile
	src/client/console/Makefile
	src/client/console/include/Makefile
	src/client/console/libnxmc/Makefile
	src/client/console/nxmc/Makefile
	src/client/console/nxmc/rc/Makefile
	src/client/console/nxmc/rc/icons/Makefile
	src/client/console/nxmc/rc/icons/normal/Makefile
	src/client/console/nxmc/rc/icons/small/Makefile
	src/client/console/plugins/Makefile
	src/client/console/plugins/AlarmBrowser/Makefile
	src/client/console/plugins/AlarmBrowser/rc/Makefile
	src/client/console/plugins/Dashboard/Makefile
	src/client/console/plugins/Dashboard/rc/Makefile
	src/client/console/plugins/ObjectBrowser/Makefile
	src/client/console/plugins/ObjectBrowser/rc/Makefile
	src/client/nxalarm/Makefile
	src/client/nxevent/Makefile
	src/client/nxpush/Makefile
	src/client/nxsms/Makefile
	src/console/Makefile
	src/client/windows/nxuilib/Makefile
	src/client/windows/nxuilib/res/Makefile
	src/client/windows/nxuilib/sounds/Makefile
	src/client/nxlexer/Makefile
	src/client/windows/nxcon/Makefile
	src/client/windows/nxcon/icons/Makefile
	src/client/windows/nxcon/res/Makefile
	src/client/windows/nxav/Makefile
	src/client/windows/nxav/res/Makefile
	src/client/windows/nxnotify/Makefile
	src/client/windows/nxnotify/res/Makefile
	src/console/PocketPC/Makefile
	src/console/PocketPC/res/Makefile
	src/nxcptest/Makefile
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
	src/server/dbdrv/oracle/Makefile
	src/server/smsdrv/Makefile
	src/server/smsdrv/generic/Makefile
	src/server/smsdrv/nxagent/Makefile
	src/server/tools/Makefile
	src/server/tools/nxget/Makefile
	src/server/tools/nxaction/Makefile
	src/server/tools/nxadm/Makefile
	src/server/tools/nxconfig/Makefile
	src/server/tools/nxconfig/res/Makefile
	src/server/tools/nxdbmgr/Makefile
	src/server/tools/nxupload/Makefile
	src/server/libnxsrv/Makefile
	src/snmp/Makefile
	src/snmp/libnxsnmp/Makefile
	src/snmp/nxmibc/Makefile
	src/snmp/nxsnmpget/Makefile
	src/snmp/nxsnmpset/Makefile
	src/snmp/nxsnmpwalk/Makefile
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
	src/agent/subagents/netbsd/Makefile
	src/agent/subagents/ipso/Makefile
	src/agent/subagents/netware/Makefile
	src/agent/subagents/winnt/Makefile
	src/agent/subagents/win9x/Makefile
	src/agent/subagents/winperf/Makefile
	src/agent/subagents/wmi/Makefile
	src/agent/subagents/portCheck/Makefile
	src/agent/subagents/ping/Makefile
	src/agent/subagents/sms/Makefile
	src/agent/subagents/ups/Makefile
	src/agent/subagents/ecs/Makefile
	src/agent/subagents/odbcquery/Makefile
	src/agent/subagents/logwatch/Makefile
	src/webui/nxhttpd/Makefile
	src/webui/nxhttpd/static/Makefile
	src/webui/nxhttpd/static/images/Makefile
	src/webui/nxhttpd/static/images/buttons/Makefile
	src/webui/nxhttpd/static/images/buttons/normal/Makefile
	src/webui/nxhttpd/static/images/buttons/pressed/Makefile
	src/webui/nxhttpd/static/images/ctrlpanel/Makefile
	src/webui/nxhttpd/static/images/objects/Makefile
	src/webui/nxhttpd/static/images/status/Makefile
	tools/Makefile
])

if test -d src/private; then
	PRIVATE=`cat src/private/configure.files`
	AC_CONFIG_FILES([$PRIVATE])
fi

AC_OUTPUT

echo "Updating libtool"
sed 's,$SED -e "/${host}-//g",$SED -e "s/${host}-//g",g' < ./libtool > libtool.new && mv libtool.new libtool && chmod +x ./libtool

# On HP-UX, libtool passes +b instead of -Wl,+b even if gcc/g++ used as linker 
if test "x$CC" = "xgcc" && test "x$PLATFORM" = "xHP-UX"; then
	sed "s,hardcode_libdir_flag_spec_ld=,#hardcode_libdir_flag_spec_ld=,g" < ./libtool > libtool.new && mv libtool.new libtool && chmod +x ./libtool
fi

# Patch config.status
if test -d src/private; then
	PRIVATE=`cat src/private/configure.files`
	$PERL tools/patch_config.status.pl $PRIVATE < config.status > config.status.temp && mv config.status.temp config.status && chmod +x ./config.status
fi

#--------------------------------------------------------------------
# Print summary
#--------------------------------------------------------------------

echo
echo
echo
echo '---------------------------------------------------------------------'
echo '                         Configure results'
echo '---------------------------------------------------------------------'
echo
	echo "Prefix                  : ${prefix}"
if test "x${BUILD_SERVER}" = "xyes"; then
	echo "Build Server            : YES"
else
	echo "Build Server            : NO"
fi
if test "x${ac_cv_lib_crypto_RSA_new}" = "xyes"; then
	echo "Encryption enabled      : YES"
else
	echo "Encryption enabled      : NO"
fi
if test "x${DB_DRIVERS}" != "x"; then
	echo "Build DB-Drivers        :${DB_DRIVERS}"
else
	echo "Build DB-Drivers        : NO"
fi
if test "x${BUILD_CLIENT}" = "xyes"; then
	echo "Build Clients           : YES"
else
	echo "Build Clients           : NO"
fi
if test "x${BUILD_CONSOLE}" = "xyes"; then
	echo "Build Console           : YES"
	echo "wxWidgets config        :" `$WXCONF --selected-config --debug=$ENABLE_DEBUG`
else
	echo "Build Console           : NO"
fi
if test "x${BUILD_NXHTTPD}" = "xyes"; then
	echo "Build Web Interface     : YES"
else
	echo "Build Web Interface     : NO"
fi
if test "x${BUILD_AGENT}" = "xyes"; then
	echo "Build Agent             : YES"
	echo "Subagents list          : ${SUBAGENT_DIRS}"
else
	echo "Build Agent             : NO"
fi
if test "x${BUILD_STATIC_AGENT}" = "xyes"; then
	echo "Build Static Agent      : YES"
	echo "Subagents list          : ${STATIC_SUBAGENT_LIST}"
else
	echo "Build Static Agent      : NO"
fi
if test "x${BUILD_UNICODE}" = "xyes"; then
	echo "Build UNICODE libraries : YES"
else
	echo "Build UNICODE libraries : NO"
fi
if test "x${HAVE_LIBEXPAT}" = "xyes"; then
	echo "XML support in NXCP     : YES"
else
	echo "XML support in NXCP     : NO"
fi
