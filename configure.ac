# $Id$
#
# NetXMS - Network Management System
# Configure script
#

AC_INIT([NetXMS], [1.1.7], [bugs@netxms.org])
AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_CONFIG_MACRO_DIR([m4])

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
ENABLE_DEBUG="no"
STATIC_BUILD="no"
DBDRIVER_SELECTED="no"
USE_PTH="no"
BUILD_SERVER="no"
BUILD_AGENT="no"
BUILD_CLIENT="no"
BUILD_NXHTTPD="no"
BUILD_STATIC_AGENT="no"
NEED_ZLIB="no"
MODULES="libnetxms tools install"
STATIC_SUBAGENT_LIST=""
SUBAGENT_DIRS=""
SUBAGENT_LIBS=""
SERVER_TOOLS=""
TOP_LEVEL_MODULES=""
CONTRIB_MODULES=""
CLIENT_COMPONENTS=""
BUILD_UNICODE="no"
ORACLE_LIBS=""
PGSQL_LIBS=""
MYSQL_LIBS=""
ODBC_LIBS=""
FORCE_INTERNAL_EXPAT="no"
USE_INTERNAL_LIBTRE="no"
FORCE_INTERNAL_LIBTRE="no"
USE_INTERNAL_ZLIB="no"
FORCE_INTERNAL_ZLIB="no"
FORCE_32BIT_BUILD="no"
LDFLAGS_PREFIX=""
LDFLAGS_SUFFIX=""
LTINIT_CC=""
LTINIT_CXX=""


#--------------------------------------------------------------------
# Common compilation flags
#--------------------------------------------------------------------

CPPFLAGS="$CPPFLAGS -D_THREAD_SAFE"


#--------------------------------------------------------------------
# Parse command line parameters
#--------------------------------------------------------------------

AC_ARG_WITH(server,
[AS_HELP_STRING(--with-server,build server)],
[
	COMPONENTS="$COMPONENTS sqlite snmp server agent"
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

AC_ARG_WITH(agent,
[AS_HELP_STRING(--with-agent,build agent)],
[
	COMPONENTS="$COMPONENTS sqlite agent"
])

AC_ARG_WITH(static-agent,
[AS_HELP_STRING(--with-static-agent,build statically linked agent)],
[
	COMPONENTS="$COMPONENTS sqlite static-agent"
])

AC_ARG_WITH(ipso-agent,
[AS_HELP_STRING(--with-ipso-agent,build statically linked IPSO agent)],
[
	COMPONENTS="$COMPONENTS ipso-agent"
])

AC_ARG_WITH(webui,
[AS_HELP_STRING(--with-webui,build web interface)],
[
	COMPONENTS="$COMPONENTS webui"
])

AC_ARG_WITH(mdebug,
[AS_HELP_STRING(--with-mdebug,use debug version of memory allocation functions)],
[
	CPPFLAGS="$CPPFLAGS -DNETXMS_MEMORY_DEBUG"
])

AC_ARG_WITH(internal-libexpat,
[AS_HELP_STRING(--with-internal-libexpat,force use of bundled libexpat)],
[
	FORCE_INTERNAL_EXPAT="yes"
])

AC_ARG_WITH(internal-libtre,
[AS_HELP_STRING(--with-internal-libtre,force use of bundled libtre)],
[
	FORCE_INTERNAL_LIBTRE="yes"
])

AC_ARG_WITH(internal-zlib,
[AS_HELP_STRING(--with-internal-zlib,force use of bundled zlib)],
[
	FORCE_INTERNAL_ZLIB="yes"
])

AC_ARG_WITH(sqlite,
[AS_HELP_STRING(--with-sqlite,build SQLite database driver)],
[ if test "x$withval" != "xno" ; then
	COMPONENTS="$COMPONENTS sqlite_drv"
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
		if test -x "$withval/lib" ; then
			LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
			LDFLAGS="$LDFLAGS -L${withval}/lib"
		else
			LD_RUN_PATH="${withval}${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
			LDFLAGS="$LDFLAGS -L${withval}"
		fi
		if test -x "$withval/include" ; then
			CPPFLAGS="$CPPFLAGS -I${withval}/include"
		fi
		if test -x "$withval/sdk/include" ; then
			CPPFLAGS="$CPPFLAGS -I${withval}/sdk/include"
		fi
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

AC_ARG_WITH(db2,
[AS_HELP_STRING(--with-db2,build DB2 database driver)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		if test `uname -m` = 'i386' || test `uname -m` = 'i686'; then
			DB2_LIB_DIR=lib32
		else
			DB2_LIB_DIR=lib64
		fi
		if test -x "$withval/${DB2_LIB_DIR}" ; then
			LD_RUN_PATH="${withval}/${DB2_LIB_DIR}${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
			LDFLAGS="$LDFLAGS -L${withval}/${DB2_LIB_DIR}"
		else
			LD_RUN_PATH="${withval}${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
			LDFLAGS="$LDFLAGS -L${withval}"
		fi
		if test -x "$withval/include" ; then
			CPPFLAGS="$CPPFLAGS -I${withval}/include"
		fi

		unset DB2_LIB_DIR
	fi
	COMPONENTS="$COMPONENTS db2"
	DBDRIVER_SELECTED="yes"
fi ])

AC_ARG_WITH(informix,
[AS_HELP_STRING(--with-informix,build Informix database driver)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		if test -x "$withval/lib/cli" ; then
			LD_RUN_PATH="${withval}/lib/cli${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
			LDFLAGS="$LDFLAGS -L${withval}/lib/cli"
			if test -x "$withval/lib/esql" ; then
				LD_RUN_PATH="${withval}/lib/esql${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
				LDFLAGS="$LDFLAGS -L${withval}/lib/esql"
			fi
		else
			LD_RUN_PATH="${withval}${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
			LDFLAGS="$LDFLAGS -L${withval}"
		fi
		if test -x "$withval/incl/cli" ; then
			CPPFLAGS="$CPPFLAGS -I${withval}/incl/cli"
		fi
	fi
	COMPONENTS="$COMPONENTS informix"
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
			CPPFLAGS="$CPPFLAGS -I/opt/openssl/include"
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

AC_ARG_ENABLE(unicode,
[AS_HELP_STRING(--enable-unicode,enable UNICODE build)],
[
	BUILD_UNICODE="yes"
	CPPFLAGS="$CPPFLAGS -DUNICODE"
])

AC_ARG_ENABLE(debug,
[AS_HELP_STRING(--enable-debug,enable additional debugging functionality)],
[
	ENABLE_DEBUG="yes"
])

AC_ARG_ENABLE(64bit,
[AS_HELP_STRING(--disable-64bit,disable 64bit build (force 32bit build event on 64bit machine))],
[
	FORCE_32BIT_BUILD="yes"
])

AC_ARG_WITH(dist,
[AS_HELP_STRING(--with-dist,for maintainers only)],
	DB_DRIVERS=" mysql pgsql odbc mssql sqlite oracle db2 informix"
	MODULES="libexpat libtre zlib libnetxms install sqlite snmp libnxsl libnxlp db server agent libnxmap libnxcl client nxscript tools webui"
	SUBAGENT_DIRS="linux freebsd openbsd netbsd sunos aix ipso hpux odbcquery informix"
	NXCONFIG="nxconfig"
	TOP_LEVEL_MODULES="sql images"
	CONTRIB_MODULES="mibs backgrounds"
	CLIENT_COMPONENTS="nxalarm nxevent nxlexer nxpush nxsms scilexer windows"
	SERVER_TOOLS="nxconfig"
)


#--------------------------------------------------------------------
# Validate command line parameters and set various checking options
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "sqlite"
if test $? = 0; then
	MODULES="$MODULES sqlite"
fi

check_substr "$COMPONENTS" "static-agent"
if test $? = 0; then
	if test "x$COMPONENTS" != "x sqlite static-agent"; then
		AC_MSG_ERROR(Static agent and other components are mutually exclusive.)
	fi
	STATIC_BUILD="yes"
	BUILD_STATIC_AGENT="yes"
	MODULES="$MODULES libnxlp agent"
	DISABLE_ICONV="yes"
	CPPFLAGS="$CPPFLAGS -D_STATIC_AGENT"
	STATIC_SUBAGENT_LIST="ecs logwatch ping portcheck ups"
	SUBAGENT_LIBS="../subagents/ecs/libnsm_ecs.la ../subagents/logwatch/libnsm_logwatch.la ../subagents/ping/libnsm_ping.la ../subagents/portCheck/libnsm_portCheck.la ../subagents/ups/libnsm_ups.la"

	case "$PLATFORM" in
		Linux)
			SUBAGENT_DIRS="linux"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/linux/libnsm_linux.la"
			STATIC_SUBAGENT_LIST="linux $STATIC_SUBAGENT_LIST"
			;;
		FreeBSD)
			SUBAGENT_DIRS="freebsd"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/freebsd/libnsm_freebsd.la"
			STATIC_SUBAGENT_LIST="freebsd $STATIC_SUBAGENT_LIST"
			;;
		OpenBSD)
			SUBAGENT_DIRS="openbsd"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/openbsd/libnsm_openbsd.la"
			STATIC_SUBAGENT_LIST="openbsd $STATIC_SUBAGENT_LIST"
			;;
		NetBSD)
			SUBAGENT_DIRS="netbsd"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/netbsd/libnsm_netbsd.la"
			STATIC_SUBAGENT_LIST="netbsd $STATIC_SUBAGENT_LIST"
			;;
		SunOS)
			SUBAGENT_DIRS="sunos"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/sunos/libnsm_sunos.la"
			STATIC_SUBAGENT_LIST="sunos $STATIC_SUBAGENT_LIST"
			;;
		AIX)
			SUBAGENT_DIRS="aix"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/aix/libnsm_aix.la"
			STATIC_SUBAGENT_LIST="aix $STATIC_SUBAGENT_LIST"
			;;
		HP-UX)
			SUBAGENT_DIRS="hpux"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/hpux/libnsm_hpux.la"
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
		AC_MSG_ERROR(IPSO agent and other components are mutually exclusive.)
	fi
	STATIC_BUILD="yes"
	BUILD_STATIC_AGENT="yes"
	DISABLE_ICONV="yes"
	USE_PTH="yes"
	MODULES="$MODULES libnxlp agent"
	CPPFLAGS="$CPPFLAGS -D_STATIC_AGENT -D_IPSO -D_USE_GNU_PTH -DSQLITE_THREADSAFE=0 -I/usr/local/include"
	LDFLAGS="$LDFLAGS -all-static"
	STATIC_SUBAGENT_LIST="ecs ipso logwatch ping portcheck ups"
	SUBAGENT_DIRS="ipso"
	SUBAGENT_LIBS="../subagents/ecs/libnsm_ecs.la ../subagents/ipso/libnsm_ipso.la ../subagents/logwatch/libnsm_logwatch.la ../subagents/ping/libnsm_ping.la ../subagents/portCheck/libnsm_portCheck.la ../subagents/ups/libnsm_ups.la"
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
	MODULES="$MODULES libnxmap libnxsl libnxlp db server nxscript"
	TOP_LEVEL_MODULES="$TOP_LEVEL_MODULES sql images"
	CONTRIB_MODULES="$CONTRIB_MODULES mibs backgrounds"
fi

check_substr "$COMPONENTS" "client"
if test $? = 0; then
	BUILD_CLIENT="yes"
	NEED_ZLIB="yes"
	MODULES="$MODULES libnxmap libnxcl client"
	CLIENT_COMPONENTS="$CLIENT_COMPONENTS nxalarm nxevent nxpush nxsms"
fi

check_substr "$COMPONENTS" "agent"
if test $? = 0; then
	BUILD_AGENT="yes"
	MODULES="$MODULES libnxlp db agent"

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

check_substr "$COMPONENTS" "webui"
if test $? = 0; then
	BUILD_NXHTTPD="yes"
	MODULES="$MODULES libnxmap libnxcl webui"
fi

check_substr "$COMPONENTS" "mysql"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS mysql"
fi

check_substr "$COMPONENTS" "pgsql"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS pgsql"
fi

check_substr "$COMPONENTS" "sqlite_drv"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS sqlite"
fi

check_substr "$COMPONENTS" "odbc"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS odbc"
	SUBAGENT_DIRS="$SUBAGENT_DIRS odbcquery"
fi

check_substr "$COMPONENTS" "oracle"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS oracle"
fi

check_substr "$COMPONENTS" "db2"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS db2"
fi

check_substr "$COMPONENTS" "informix"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS informix"
	SUBAGENT_DIRS="$SUBAGENT_DIRS informix"
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
      LDFLAGS_PREFIX="-no-undefined -Wl,-brtl $LDFLAGS"
      ;;
   HP-UX)
      CPPFLAGS="$CPPFLAGS -D_HPUX -D_HPUX_SOURCE -D_POSIX_PTHREAD_SEMANTICS -D_XOPEN_SOURCE=500 -D_XOPEN_SOURCE_EXTENDED=1 -D_REENTRANT -D_INCLUDE_LONGLONG"
      if test "x$CXX" = "xaCC"; then
	 CFLAGS="$CFLAGS -AC99"
         CXXFLAGS="-mt $CXXFLAGS +W749 +W829"
         LDFLAGS="$LDFLAGS -mt"
      fi
      ;;
   SunOS)
      CPPFLAGS="$CPPFLAGS -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS"
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
#  * Disable C++ exceptions and RTTI
#  * Turn on 64bit mode on HP-UX/Itanium
#  * Turn on 64bit mode on AIX
#--------------------------------------------------------------------

if test "x$CC" = "xgcc" ; then

	AC_LANG_PUSH([C++])

	AC_MSG_CHECKING(whether C++ compiler accepts -fno-rtti)
	OLD_CXXFLAGS="$CXXFLAGS"
	CXXFLAGS="$CXXFLAGS -fno-rtti"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
		[ AC_MSG_RESULT(yes) ], 
		[ 
			CXXFLAGS="$OLD_CXXFLAGS"
			AC_MSG_RESULT(no)
		])

	AC_MSG_CHECKING(whether C++ compiler accepts -fno-exceptions)
	OLD_CXXFLAGS="$CXXFLAGS"
	CXXFLAGS="$CXXFLAGS -fno-exceptions"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
		[ AC_MSG_RESULT(yes) ], 
		[ 
			CXXFLAGS="$OLD_CXXFLAGS"
			AC_MSG_RESULT(no)
		])

	AC_LANG_POP([C++])

	AC_MSG_CHECKING(whether C compiler accepts -Wno-unused-result)
	OLD_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS -Wno-unused-result"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
		[ AC_MSG_RESULT(yes) ], 
		[ 
			CPPFLAGS="$OLD_CPPFLAGS"
			AC_MSG_RESULT(no)
		])

	if test "x$PLATFORM" = "xHP-UX"; then
		if test "x$FORCE_32BIT_BUILD" = "xyes"; then
			AC_MSG_CHECKING(whether C compiler accepts -milp32)
			OLD_CPPFLAGS="$CPPFLAGS"
			CPPFLAGS="$CPPFLAGS -milp32"
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
				[ 
					LDFLAGS="-milp32 $LDFLAGS"
					AC_MSG_RESULT(yes) 
				], 
				[ 
					CPPFLAGS="$OLD_CPPFLAGS"
					AC_MSG_RESULT(no)
				])
		else
			AC_MSG_CHECKING(whether C compiler accepts -mlp64)
			OLD_CPPFLAGS="$CPPFLAGS"
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
	fi

	if test "x$PLATFORM" = "xAIX"; then
		if test "x$FORCE_32BIT_BUILD" = "xyes"; then
			AC_MSG_CHECKING(whether C compiler accepts -maix32)
			OLD_CPPFLAGS="$CPPFLAGS"
			CPPFLAGS="$CPPFLAGS -maix32"
			AC_RUN_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
				[ 
					LDFLAGS="-maix32 $LDFLAGS"
					LTINIT_CC="gcc -maix32"
					LTINIT_CXX="g++ -maix32"
					OBJECT_MODE=32
					AC_MSG_RESULT(yes) 
				], 
				[ 
					CPPFLAGS="$OLD_CPPFLAGS"
					AC_MSG_RESULT(no)
				])
		else
			AC_MSG_CHECKING(whether C compiler accepts -maix64)
			OLD_CPPFLAGS="$CPPFLAGS"
			CPPFLAGS="$CPPFLAGS -maix64"
			AC_RUN_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
				[ 
					LDFLAGS="-maix64 $LDFLAGS"
					LTINIT_CC="gcc -maix64"
					LTINIT_CXX="g++ -maix64"
					OBJECT_MODE=64
					AC_MSG_RESULT(yes) 
				], 
				[ 
					CPPFLAGS="$OLD_CPPFLAGS"
					AC_MSG_RESULT(no)
				])
		fi
	fi

	if test "x$ENABLE_DEBUG" = "xyes"; then
		CPPFLAGS="$CPPFLAGS -ggdb3"
	fi
fi

if test "x$CXX" = "xaCC" ; then
	if test "x$FORCE_32BIT_BUILD" = "xyes"; then
		AC_MSG_CHECKING(whether C compiler accepts +DD32)
		OLD_CPPFLAGS="$CPPFLAGS"
		CPPFLAGS="+DD32 $CPPFLAGS"
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
			[ 
				LDFLAGS="+DD32 $LDFLAGS"
				AC_MSG_RESULT(yes) 
			], 
			[ 
				CPPFLAGS="$OLD_CPPFLAGS"
				AC_MSG_RESULT(no)
			])
	else
		AC_MSG_CHECKING(whether C compiler accepts +DD64)
		OLD_CPPFLAGS="$CPPFLAGS"
		CPPFLAGS="+DD64 $CPPFLAGS"
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
			[ 
				LDFLAGS="+DD64 $LDFLAGS"
				AC_MSG_RESULT(yes) 
			], 
			[ 
				CPPFLAGS="$OLD_CPPFLAGS"
				AC_MSG_RESULT(no)
			])
	fi
fi


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

if test "x$LTINIT_CC" != "x"; then
	SAVED_CC="$CC"
	CC="$LTINIT_CC"
fi
if test "x$LTINIT_CXX" != "x"; then
	SAVED_CXX="$CXX"
	CXX="$LTINIT_CXX"
fi

LT_PREREQ([2.2])
LT_INIT([dlopen])

if test "x$SAVED_CC" != "x"; then
	CC="$SAVED_CC"
fi
if test "x$SAVED_CXX" != "x"; then
	CXX="$SAVED_CXX"
fi


#--------------------------------------------------------------------
# Additional include and library directories
#--------------------------------------------------------------------

if test "x$CXX" != "xaCC"; then
	if test -d /usr/local/include; then
		if test "x$CC" = "xgcc"; then
			AC_MSG_CHECKING(whether -I/usr/local/include needed)
			OLD_CPPFLAGS="$CPPFLAGS"
			CPPFLAGS="$CPPFLAGS -Werror -I/usr/local/include"
			AC_TRY_COMPILE([],[],
				[
					CPPFLAGS="$OLD_CPPFLAGS -I/usr/local/include"
					AC_MSG_RESULT(yes)
				],
				[
					CPPFLAGS="$OLD_CPPFLAGS"
					AC_MSG_RESULT(no)
				]
			)
		else
			CPPFLAGS="$CPPFLAGS -I/usr/local/include"
		fi
	fi

	if test -d /usr/kerberos/include; then
		CPPFLAGS="$CPPFLAGS -I/usr/kerberos/include"
	fi

	if test -d /usr/local/lib; then
		LDFLAGS="$LDFLAGS -L/usr/local/lib"
	fi
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
			PTHREAD_LIBS="-pthread",
			AC_MSG_RESULT(no))
		if test "x$PTHREAD_LIBS" = "xerror"; then
			AC_CHECK_LIB(pthread, pthread_attr_init,
				PTHREAD_LIBS="-lpthread")
		fi
		# On HP-UX (at least 11.00) pthread library contains
		# __pthread_attr_init_system instead of pthread_attr_init
		if test "x$PTHREAD_LIBS" = "xerror"; then
			AC_CHECK_LIB(pthread, __pthread_attr_init_system,
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
	LIBS="$LIBS $PTHREAD_LIBS"

	# Extensions to posix threads
	AC_CHECK_HEADERS([pthread_np.h],,,[
#include <pthread.h>
	])
	AC_CHECK_FUNCS([pthread_cond_reltimedwait_np])

	# Check for recursive mutexes
	AC_CHECK_FUNCS([pthread_mutexattr_settype __pthread_mutexattr_settype pthread_mutexattr_setkind_np])
	AC_CHECK_DECLS([pthread_mutexattr_settype, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_RECURSIVE_NP, MUTEX_TYPE_COUNTING_FAST],,,[
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
# Checks for header files
#--------------------------------------------------------------------

AC_CHECK_HEADERS([sys/types.h sys/stat.h unistd.h stdarg.h fcntl.h alloca.h])
AC_CHECK_HEADERS([sys/int_types.h time.h sys/time.h sys/utsname.h sys/wait.h])
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h net/nh.h sys/socket.h])
AC_CHECK_HEADERS([fcntl.h dirent.h sys/ioctl.h sys/sockio.h poll.h termios.h])
AC_CHECK_HEADERS([inttypes.h memory.h stdint.h stdlib.h strings.h string.h])
AC_CHECK_HEADERS([readline/readline.h byteswap.h sys/select.h dlfcn.h])
AC_CHECK_HEADERS([sys/sysctl.h sys/param.h sys/user.h vm/vm_param.h syslog.h getopt.h])
AC_CHECK_HEADERS([net/if.h net/if_arp.h net/if_dl.h net/if_types.h],,,
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
# Checks for other libs.
#--------------------------------------------------------------------

AC_CHECK_LIB(xnet, accept)
AC_CHECK_LIB(socket, if_nameindex)
AC_CHECK_LIB(dl, dlopen)
AC_CHECK_LIB(kstat, kstat_open)
AC_CHECK_LIB(gnugetopt, getopt_long)

if test "x$BUILD_SERVER" = "xyes" ; then
   AC_CHECK_LIB(termcap, tgetstr, [], [AC_CHECK_LIB(ncurses, tgetstr, [], AC_CHECK_LIB(curses, tgetstr))])
   AC_CHECK_LIB(readline, readline, [], [AC_CHECK_LIB(edit, readline)])
   AC_CHECK_FUNCS([readline])

   # Check if passing rl_insert to rl_bind_key needs cast
   if test "$ac_cv_func_readline" = "yes"; then
      AC_LANG_PUSH([C++])

      AC_MSG_CHECKING(for rl_insert cast)

      AC_TRY_COMPILE(
         [
#include <stdio.h>
#include <readline/readline.h>
         ],[ rl_bind_key('\t', (Function *)rl_insert); ],
         [ 
            AC_MSG_RESULT((Function *)) 
            AC_DEFINE(RL_INSERT_CAST,(Function *),Required cast for rl_insert)
         ],[
         AC_TRY_COMPILE(
            [
#include <stdio.h>
#include <readline/readline.h>
            ],[ rl_bind_key('\t', (rl_command_func_t *)rl_insert); ],
            [ 
               AC_MSG_RESULT((rl_command_func_t *)) 
               AC_DEFINE(RL_INSERT_CAST,(rl_command_func_t *),Required cast for rl_insert)
            ],[ 
               AC_MSG_RESULT(not needed) 
               AC_DEFINE(RL_INSERT_CAST,,Required cast for rl_insert)
            ]
         )
         ]
      )

      AC_LANG_POP([C++])
   fi
fi

if test "x$NEED_ZLIB" = "xyes"; then
	if test "x$FORCE_INTERNAL_ZLIB" = "xyes"; then
		HAVE_ZLIB=no
	else
		HAVE_ZLIB=yes
		AC_CHECK_HEADER(zlib.h,,HAVE_ZLIB=no)
		if test "x$HAVE_ZLIB" = "xyes"; then
			AC_CHECK_LIB(z, deflate, [], [ HAVE_ZLIB=no ])
		fi
	fi
	if test "x$HAVE_ZLIB" = "xno"; then
		MODULES="zlib $MODULES"
	fi
fi

if test "x$FORCE_INTERNAL_EXPAT" = "xyes"; then
	HAVE_LIBEXPAT=no
else
	HAVE_LIBEXPAT=yes
	AC_CHECK_HEADER(expat.h,,HAVE_LIBEXPAT=no)
	AC_CHECK_LIB(expat, XML_Parse, [], [ HAVE_LIBEXPAT=no ])
fi
if test "x$HAVE_LIBEXPAT" = "xno"; then
	MODULES="libexpat $MODULES"
fi

if test "x$FORCE_INTERNAL_LIBTRE" = "xyes"; then
	HAVE_LIBTRE=no
else
	HAVE_LIBTRE=yes
	AC_CHECK_HEADER(tre/regex.h,,HAVE_LIBTRE=no)
	AC_CHECK_LIB(tre, tre_compile, [], [ HAVE_LIBTRE=no ])
fi
if test "x$HAVE_LIBTRE" = "xno"; then
	MODULES="libtre $MODULES"
	AC_DEFINE(USE_BUNDLED_LIBTRE,1,Define to 1 if bundled libtre is used)

	tre_version_1='0'
	tre_version_2='8'
	tre_version_3='0'
	tre_version=$tre_version_1.$tre_version_2.$tre_version_3
	AC_DEFINE_UNQUOTED(TRE_VERSION,  "$tre_version",  [ TRE version string.  ])
	AC_DEFINE_UNQUOTED(TRE_VERSION_1, $tre_version_1, [ TRE version level 1. ])
	AC_DEFINE_UNQUOTED(TRE_VERSION_2, $tre_version_2, [ TRE version level 2. ])
	AC_DEFINE_UNQUOTED(TRE_VERSION_3, $tre_version_3, [ TRE version level 3. ])
	AC_SUBST(TRE_VERSION, $tre_version)

	AC_DEFINE(TRE_REGEX_T_FIELD,value,[Define to a field in the regex_t struct where TRE should store a pointer to the internal tre_tnfa_t structure])
	AC_DEFINE(TRE_APPROX, 1, [ Define if you want to enable approximate matching functionality. ])
fi

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

AC_CHECK_TYPES([long long, unsigned long long, int64_t, uint64_t, u_int64_t, uuid_t])
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

AC_CHECK_FUNCS([gettimeofday memmove memset bcopy strchr strcspn strdup strerror])
AC_CHECK_FUNCS([strrchr strtol strtoul strtoll strtoull])
AC_CHECK_FUNCS([if_nametoindex daemon mmap strerror_r scandir uname poll])
AC_CHECK_FUNCS([usleep nanosleep getopt_long gmtime_r localtime_r lstat64])

AC_CHECK_DECLS([nanosleep])

# Check if strerror_r is POSIX-compliant
if test "$ac_cv_func_strerror_r" = "yes"; then
	AC_MSG_CHECKING(if strerror_r is POSIX compliant)
	AC_LANG_PUSH([C++])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
	]],[[
		int e;
		char buffer[256];
		e = strerror_r(1, buffer, 256);
		return e;
	]])
	],[
		AC_MSG_RESULT(yes)
		AC_DEFINE(HAVE_POSIX_STRERROR_R,1,Define to 1 if strerror_r function is POSIX compliant)
	],[
		AC_MSG_RESULT(no)
		AC_DEFINE(HAVE_POSIX_STRERROR_R,0,Define to 1 if strerror_r function is POSIX compliant)
	])
	AC_LANG_POP([C++])
fi

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
   timezone(0, 0);
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
   struct tm t;
   return t.tm_gmtoff;
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

# asprintf, scprintf, etc.
AC_CHECK_FUNCS([asprintf aswprintf vasprintf vaswprintf])
AC_CHECK_FUNCS([scprintf scwprintf vscprintf vscwprintf])

AC_MSG_CHECKING(whether snprintf returns required buffer size)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([ 
#include <stdio.h>
#include <string.h>
     ], [
     return !(snprintf(NULL, 0, "test: %d", 1000) >= 10);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(SNPRINTF_RETURNS_REQUIRED_SIZE, 1, Define to 1 if snprintf returns required buffer size in case of truncation)
  ],
  [ AC_MSG_RESULT(no) ],
  [ AC_MSG_RESULT(no) ]
)



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
AC_CHECK_DECLS([RTLD_NOW, RTLD_GLOBAL],,,[
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif
])
AC_CHECK_DECLS([va_copy, __va_copy],,,[
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
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
# HP-UX specific checks
#--------------------------------------------------------------------

if test "x$PLATFORM" = "xHP-UX"; then
	AC_CHECK_HEADERS([sys/mib.h],,,[[ ]])
	AC_CHECK_DECLS([SIOCGIFNAME, SIOCGIFINDEX],,,[
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
	])
	AC_CHECK_DECLS([ID_ifXEntry, open_mib_64],,,[
#if HAVE_SYS_MIB_H
#include <sys/mib.h>
#endif
	])

	AC_MSG_CHECKING(if pst_diskinfo has read/write stats fields)
	AC_LANG_PUSH([C++])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ 
#include <sys/pstat.h>
			]], [[ 
pst_diskinfo d;
int x;
x = (int)d.psd_dkread;
x = (int)d.psd_dkwrite;
			]])],
		[ 
			AC_DEFINE([HAVE_DISKINFO_RWSTATS],[1],[Define to 1 if pst_diskinfo has read/write stats fields])
			AC_MSG_RESULT(yes) 
		], 
		[ 
			AC_MSG_RESULT(no)
		])
	AC_LANG_POP([C++])
fi


#--------------------------------------------------------------------
# *BSD specific checks
#--------------------------------------------------------------------

AC_CHECK_LIB(kvm, kvm_open)
AC_CHECK_FUNCS([kvm_getswapinfo])
AC_CHECK_TYPES([struct kinfo_proc2],,,[
#if HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
])


#--------------------------------------------------------------------
# FreeBSD specific checks
#--------------------------------------------------------------------

if test "x$PLATFORM" = "xFreeBSD"; then
	AC_CHECK_DECLS([RTF_WASCLONED],[],[],[
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <net/route.h>
	])
fi


#--------------------------------------------------------------------
# OpenBSD specific checks
#--------------------------------------------------------------------

if test "x$PLATFORM" = "xOpenBSD"; then
	AC_MSG_CHECKING(if kvm_getprocs requires sizeof kinfo_proc argument)
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/param.h>
#include <sys/proc.h>
#include <kvm.h>
#include <sys/user.h>
        ]],[[
		int count;
		kvm_getprocs(NULL, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc), &count);
		return 0;
        ]])
        ],[
                AC_MSG_RESULT(yes)
                AC_DEFINE(KVM_GETPROCS_REQUIRES_SIZEOF,1,Define to 1 if kvm_getprocs requires sizeof kinfo_proc)
        ],[
                AC_MSG_RESULT(no)
                AC_DEFINE(KVM_GETPROCS_REQUIRES_SIZEOF,0,Define to 1 if kvm_getprocs requires sizeof kinfo_proc)
        ])

	AC_MSG_CHECKING(if struct kinfo_proc has member kp_proc)
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/param.h>
#include <sys/proc.h>
#include <kvm.h>
#include <sys/user.h>
        ]],[[
		struct kinfo_proc p;
		return p.kp_proc.p_pid;
        ]])
        ],[
                AC_MSG_RESULT(yes)
                AC_DEFINE(KINFO_PROC_HAS_KP_PROC,1,Define to 1 if struct kinfo_proc has member kp_proc)
        ],[
                AC_MSG_RESULT(no)
                AC_DEFINE(KINFO_PROC_HAS_KP_PROC,0,Define to 1 if struct kinfo_proc has member kp_proc)
        ])
fi


#--------------------------------------------------------------------
# Check for UNICODE stuff
#--------------------------------------------------------------------

AC_CHECK_HEADERS([wchar.h wctype.h iconv.h])
AC_CHECK_TYPES([wchar_t, wint_t],,,[
#if HAVE_WCHAR_H
#include <wchar.h>
#endif
])
AC_CHECK_SIZEOF(wchar_t)
AC_CHECK_FUNCS([wcslen wcsdup wcsncpy wcstoll wcstoull towupper wcserror wcserror_r])
AC_CHECK_FUNCS([wfopen wopen wstat wgetenv wrename wunlink])
AC_CHECK_FUNCS([wfopen wopen wstat wgetenv wrename wunlink])

if test "x$DISABLE_ICONV" != "xyes"; then
	AC_CHECK_LIB(iconv, libiconv_open,
		[ ac_found_iconv=yes
		  LIBS="$LIBS -liconv"
		  AC_CHECK_FUNCS(libiconv, ac_found_iconv=yes, ac_found_iconv=no)
		],
		[
			AC_CHECK_LIB(iconv, iconv, 
				[ ac_found_iconv=yes
			          LIBS="$LIBS -liconv"
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

AC_MSG_CHECKING(whether iconv supports UTF-16)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","UTF-16")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_UTF_16, 1, Define to 1 if iconv supports UTF-16)
    valid_ucs2_code="UTF-16"
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

AC_MSG_CHECKING(whether iconv supports UTF-32)
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([
#if HAVE_ICONV_H
#include <iconv.h>
#endif
     ], [
     return iconv_open("UTF-8","UTF-32")==(iconv_t)(-1);
     ])
  ],
  [ AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_ICONV_UTF_32, 1, Define to 1 if iconv supports UTF-32)
    valid_ucs4_code="UTF-32"
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
  AC_LANG_PUSH([C++])
  AC_CACHE_CHECK(for iconv declaration, ac_cv_iconv_const,
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdlib.h>
#if HAVE_ICONV_H
#include <iconv.h>
#endif
	]],
        [[
		char **p = NULL;
		iconv(0, (const char **)p, 0, 0, 0);
	]])],
      [ac_cv_iconv_const=const],
      [ac_cv_iconv_const=])])
  AC_LANG_POP([C++])
  AC_DEFINE_UNQUOTED([ICONV_CONST], $ac_cv_iconv_const,
    [Define as const if the declaration of iconv() needs const.])
fi

if test "x$ac_cv_type_wchar_t" = "xyes"; then
	if test $ac_cv_sizeof_wchar_t -eq 2; then
		AC_DEFINE(UNICODE_UCS2, 1, Define to 1 if you have 2-byte wchar_t)
	fi

	if test $ac_cv_sizeof_wchar_t -eq 4; then
		AC_DEFINE(UNICODE_UCS4, 1, Define to 1 if you have 4-byte wchar_t)
	fi

	if test "x$USE_INTERNAL_LIBTRE" = "xyes"; then
		AC_DEFINE(TRE_WCHAR, 1,    [ Define to enable wide character support in libtre.])
	fi
else
	if test "x$BUILD_UNICODE" = "xyes"; then
		AC_MSG_ERROR(wchar_t is required for UNICODE build)
	fi
fi


#--------------------------------------------------------------------
# MySQL
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "mysql"
if test $? = 0; then
	AC_CHECK_LIB(mysqlclient, mysql_init, [MYSQL_LIBS="-lmysqlclient"],
		[AC_MSG_ERROR(libmysqlclient is requred for MySQL support)])

	OLD_LIBS="$LIBS"
	LIBS="$LIBS $MYSQL_LIBS"

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
	LIBS="$OLD_LIBS"
fi


#--------------------------------------------------------------------
# PostgreSQL
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "pgsql"
if test $? = 0; then
	AC_CHECK_LIB(m, floor)
	AC_CHECK_LIB(pq, PQconnectdb, [PGSQL_LIBS="-lpq"],
		[AC_MSG_ERROR(libpq is required for PostgreSQL support)])        

	AC_MSG_CHECKING(whether postgresql clients can run)
	OLD_LIBS="$LIBS"
	LIBS="$LIBS $PGSQL_LIBS"
	AC_RUN_IFELSE([AC_LANG_SOURCE([[
			#include <stdio.h>
			#include <libpq-fe.h>
			int main(void)
			{
				PGconn *a = PQconnectdb("");
				return 0;
			}
		]])],[AC_MSG_RESULT(yes)],[
			AC_MSG_RESULT(no)
			AC_MSG_ERROR(Your PostgreSQL client libraries aren't properly installed)
		],[])    
	LIBS="$OLD_LIBS"
	AC_CHECK_LIB(pq, PQfformat, [ AC_DEFINE(HAVE_PQFFORMAT, 1, PQfformat not exist in 7.3) ])
fi


#--------------------------------------------------------------------
# ODBC
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "odbc"
if test $? = 0; then
	AC_CHECK_LIB(odbc, SQLAllocHandle, [ODBC_LIBS="-lodbc"],
		[AC_MSG_ERROR(libodbc is required for ODBC support)])
fi


#--------------------------------------------------------------------
# Oracle
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "oracle"
if test $? = 0; then
	AC_CHECK_HEADER(oci.h,,AC_MSG_ERROR([*** Cannot find oci.h - check your Oracle client installation ***]))
	AC_CHECK_LIB(nnz11, nzstrcpy, [NNZ_LIB=-lnnz11],
		[
			AC_CHECK_LIB(nnz10, nzstrcpy, [NNZ_LIB=-lnnz10], [], [-lclntsh])
		], [-lclntsh])
	AC_CHECK_LIB(clntsh, OCIHandleAlloc, [ORACLE_LIBS="-lclntsh $NNZ_LIB"],
		[AC_MSG_ERROR(libclntsh is required for Oracle support)], [$NNZ_LIB])
	AC_CHECK_LIB(rt, sem_destroy, [ORACLE_LIBS="$ORACLE_LIBS -lrt"], [])
fi

#--------------------------------------------------------------------
# DB2
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "db2"
if test $? = 0; then
	AC_CHECK_HEADER(sql.h,,AC_MSG_ERROR([*** Cannot find sql.h - check your DB2 client installation ***]))
	AC_CHECK_LIB(db2, SQLAllocHandle, [DB2_LIBS="-ldb2"],
		[AC_MSG_ERROR(libdb2 is required for DB2 support)])
fi

#--------------------------------------------------------------------
# Informix
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "informix"
if test $? = 0; then
	AC_CHECK_HEADER(infxcli.h,,AC_MSG_ERROR([*** Cannot find infxcli.h - check your Informix client installation ***]))
	AC_CHECK_LIB(thcli, SQLAllocHandle, [INFORMIX_LIBS="-lthcli"],
		[AC_MSG_ERROR(libthcli is required for Informix support)])
fi

#--------------------------------------------------------------------
# nxhttpd
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "webui"
if test $? = 0; then
	AC_CHECK_HEADER(gd.h,,AC_MSG_ERROR([*** GD support not installed - please install first ***]))
	AC_CHECK_LIB(gd, gdImageCreate, [], [
			AC_MSG_ERROR(libgd is required for web interface)
		])

fi


#--------------------------------------------------------------------
# Other settings
#--------------------------------------------------------------------

CPPFLAGS="$CPPFLAGS -DPREFIX=\\\"\${prefix}\\\" -DPREFIXW=L\\\"\${prefix}\\\" -DDATADIR=\\\"\${pkgdatadir}\\\" -DBINDIR=\\\"\${bindir}\\\" -DLIBDIR=\\\"\${libdir}\\\" -DPKGLIBDIR=\\\"\${pkglibdir}\\\""
LDFLAGS="$LDFLAGS_PREFIX $LDFLAGS $LDFLAGS_SUFFIX"

AM_CONDITIONAL([USE_INTERNAL_EXPAT], [test "x$HAVE_LIBEXPAT" = "xno"])
AM_CONDITIONAL([USE_INTERNAL_LIBTRE], [test "x$HAVE_LIBTRE" = "xno"])
AM_CONDITIONAL([USE_INTERNAL_ZLIB], [test "$NEED_ZLIB/$HAVE_ZLIB" = "yes/no"])


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
AC_SUBST(MYSQL_LIBS)
AC_SUBST(PGSQL_LIBS)
AC_SUBST(ORACLE_LIBS)
AC_SUBST(ODBC_LIBS)
AC_SUBST(DB2_LIBS)
AC_SUBST(INFORMIX_LIBS)
AC_SUBST(OBJECT_MODE)

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

NETXMS_LIBRARY_VERSION=1:0:0
AC_SUBST(NETXMS_LIBRARY_VERSION)


#--------------------------------------------------------------------
# Generate output files
#--------------------------------------------------------------------

AC_CONFIG_FILES([
	README
	Makefile
	contrib/Makefile
	contrib/startup/Makefile
	contrib/startup/debian/Makefile
	contrib/startup/gentoo/Makefile
	contrib/startup/redhat/Makefile
	contrib/startup/suse/Makefile
	contrib/startup/ubuntu/Makefile
	contrib/mibs/Makefile
	contrib/backgrounds/Makefile
	doc/Makefile
	doc/internal/Makefile
	doc/manuals/Makefile
	doc/misc/Makefile
	images/Makefile
	include/Makefile
	m4/Makefile
	src/Makefile
	src/agent/Makefile
	src/agent/core/Makefile
	src/agent/install/Makefile
	src/agent/subagents/Makefile
	src/agent/subagents/aix/Makefile
	src/agent/subagents/ecs/Makefile
	src/agent/subagents/freebsd/Makefile
	src/agent/subagents/hpux/Makefile
	src/agent/subagents/informix/Makefile
	src/agent/subagents/ipso/Makefile
	src/agent/subagents/linux/Makefile
	src/agent/subagents/logwatch/Makefile
	src/agent/subagents/netbsd/Makefile
	src/agent/subagents/odbcquery/Makefile
	src/agent/subagents/openbsd/Makefile
	src/agent/subagents/ping/Makefile
	src/agent/subagents/portCheck/Makefile
	src/agent/subagents/sms/Makefile
	src/agent/subagents/sunos/Makefile
	src/agent/subagents/ups/Makefile
	src/agent/subagents/winnt/Makefile
	src/agent/subagents/winperf/Makefile
	src/agent/subagents/wmi/Makefile
	src/client/Makefile
	src/client/install/Makefile
	src/client/nxalarm/Makefile
	src/client/nxevent/Makefile
	src/client/nxlexer/Makefile
	src/client/nxpush/Makefile
	src/client/nxsms/Makefile
	src/client/scilexer/Makefile
	src/client/windows/Makefile
	src/client/windows/nxav/Makefile
	src/client/windows/nxav/res/Makefile
	src/client/windows/nxcon/Makefile
	src/client/windows/nxcon/icons/Makefile
	src/client/windows/nxcon/res/Makefile
	src/client/windows/nxnotify/Makefile
	src/client/windows/nxnotify/res/Makefile
	src/client/windows/nxuilib/Makefile
	src/client/windows/nxuilib/res/Makefile
	src/client/windows/nxuilib/sounds/Makefile
	src/db/Makefile
	src/db/dbdrv/Makefile
	src/db/dbdrv/db2/Makefile
	src/db/dbdrv/informix/Makefile
	src/db/dbdrv/odbc/Makefile
	src/db/dbdrv/mssql/Makefile
	src/db/dbdrv/mysql/Makefile
	src/db/dbdrv/oracle/Makefile
	src/db/dbdrv/pgsql/Makefile
	src/db/dbdrv/sqlite/Makefile
	src/db/libnxdb/Makefile
	src/install/Makefile
	src/install/windows/Makefile
	src/libexpat/Makefile
	src/libexpat/libexpat/Makefile
	src/libnetxms/Makefile
	src/libnxcl/Makefile
	src/libnxlp/Makefile
	src/libnxmap/Makefile
	src/libnxsl/Makefile
	src/libtre/Makefile
	src/libtre/win32/Makefile
	src/nxscript/Makefile
	src/server/Makefile
	src/server/core/Makefile
	src/server/drivers/Makefile
	src/server/drivers/baystack/Makefile
	src/server/drivers/cat2900xl/Makefile
	src/server/drivers/ers8000/Makefile
	src/server/drivers/lib/Makefile
	src/server/drivers/lib/avaya-ers/Makefile
	src/server/drivers/lib/cisco/Makefile
	src/server/drivers/netscreen/Makefile
	src/server/include/Makefile
	src/server/libnxsrv/Makefile
	src/server/netxmsd/Makefile
	src/server/smsdrv/Makefile
	src/server/smsdrv/generic/Makefile
	src/server/smsdrv/nxagent/Makefile
	src/server/smsdrv/portech/Makefile
	src/server/tools/Makefile
	src/server/tools/nxaction/Makefile
	src/server/tools/nxadm/Makefile
	src/server/tools/nxap/Makefile
	src/server/tools/nxconfig/Makefile
	src/server/tools/nxconfig/res/Makefile
	src/server/tools/nxdbmgr/Makefile
	src/server/tools/nxencpasswd/Makefile
	src/server/tools/nxget/Makefile
	src/server/tools/nxupload/Makefile
	src/snmp/Makefile
	src/snmp/libnxsnmp/Makefile
	src/snmp/nxmibc/Makefile
	src/snmp/nxsnmpget/Makefile
	src/snmp/nxsnmpset/Makefile
	src/snmp/nxsnmpwalk/Makefile
	src/sqlite/Makefile
	src/tools/Makefile
	src/tools/nxcptest/Makefile
	src/tools/nxdevcfg/Makefile
	src/webui/Makefile
	src/webui/nxhttpd/Makefile
	src/webui/nxhttpd/static/Makefile
	src/webui/nxhttpd/static/images/Makefile
	src/webui/nxhttpd/static/images/buttons/Makefile
	src/webui/nxhttpd/static/images/buttons/normal/Makefile
	src/webui/nxhttpd/static/images/buttons/pressed/Makefile
	src/webui/nxhttpd/static/images/ctrlpanel/Makefile
	src/webui/nxhttpd/static/images/objects/Makefile
	src/webui/nxhttpd/static/images/status/Makefile
	src/zlib/Makefile
	sql/Makefile
	tools/Makefile
])

AC_OUTPUT

echo "Updating libtool"
sed 's,$SED -e "/${host}-//g",$SED -e "s/${host}-//g",g' < ./libtool > libtool.new && mv libtool.new libtool && chmod +x ./libtool

# On HP-UX, libtool passes +b instead of -Wl,+b even if gcc/g++ used as linker 
if test "x$CC" = "xgcc" && test "x$PLATFORM" = "xHP-UX"; then
	sed "s,hardcode_libdir_flag_spec_ld=,#hardcode_libdir_flag_spec_ld=,g" < ./libtool > libtool.new && mv libtool.new libtool && chmod +x ./libtool
fi

# On AIX, correct OBJECT_MODE must be set before linking
# To avoid manual setting by user, we add this to libtool script
if test "x$PLATFORM" = "xAIX"; then
	head -n 1 ./libtool > ./libtool.new
	echo "export OBJECT_MODE=$OBJECT_MODE" >> ./libtool.new
	tail -n +2 ./libtool >> ./libtool.new
	mv libtool.new libtool
	chmod +x ./libtool
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
	echo "UNICODE build           : YES"
else
	echo "UNICODE build           : NO"
fi
if test "x${HAVE_LIBEXPAT}" = "xno"; then
	echo "Use internal libexpat   : YES"
else
	echo "Use internal libexpat   : NO"
fi
if test "x${HAVE_LIBTRE}" = "xno"; then
	echo "Use internal libtre     : YES"
else
	echo "Use internal libtre     : NO"
fi
if test "x${NEED_ZLIB}" = "xyes"; then
	if test "x${HAVE_ZLIB}" = "xno"; then
		echo "Use internal zlib       : YES"
	else
		echo "Use internal zlib       : NO"
	fi
fi
if test "x${FORCE_32BIT_BUILD}" = "xyes"; then
	echo "Force 32bit build       : YES"
else
	echo "Force 32bit build       : NO"
fi

echo "CPPFLAGS                : ${CPPFLAGS}"
echo "CXXFLAGS                : ${CXXFLAGS}"
echo "CFLAGS                  : ${CFLAGS}"
echo "LDFLAGS                 : ${LDFLAGS}"
echo "LIBS                    : ${LIBS}"

if test "x${MYSQL_LIBS}" != "x"; then
	echo "MySQL libs              : ${MYSQL_LIBS}"
fi
if test "x${PGSQL_LIBS}" != "x"; then
	echo "PostgreSQL libs         : ${PGSQL_LIBS}"
fi
if test "x${ORACLE_LIBS}" != "x"; then
	echo "Oracle libs             : ${ORACLE_LIBS}"
fi
if test "x${ODBC_LIBS}" != "x"; then
	echo "ODBC libs               : ${ODBC_LIBS}"
fi
if test "x${DB2_LIBS}" != "x"; then
	echo "DB2 libs                : ${DB2_LIBS}"
fi
if test "x${INFORMIX_LIBS}" != "x"; then
	echo "Informix libs           : ${INFORMIX_LIBS}"
fi
