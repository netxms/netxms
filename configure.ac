#  $Id$
#
#  NetXMS - Network Management System
# Configure script
#

AC_INIT([NetXMS], [6.0.0], [bugs@netxms.org])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE([tar-ustar subdir-objects])
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

find_existing_dir()
{
	for d in "$@"; do
		if test -d "$d"; then
			__dir="$d"
			return 0;
		fi
	done
	return 1
}


#--------------------------------------------------------------------
# Macros
#--------------------------------------------------------------------

AC_DEFUN([AC_C_CHECK_OPTION],[
	AC_MSG_CHECKING(whether C compiler accepts $1)
	OLD_CFLAGS="$CFLAGS"
	if test "x$ac_compiler_gnu" = "xyes" ; then
		CFLAGS="$CFLAGS -Werror $1"
	else
		CFLAGS="$CFLAGS $1"
	fi
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
		[ 
			CFLAGS="$OLD_CFLAGS $1"
			AC_MSG_RESULT(yes) 
		], 
		[ 
			CFLAGS="$OLD_CFLAGS"
			AC_MSG_RESULT(no)
		])
])

AC_DEFUN([AC_CPP_CHECK_OPTION],[
	AC_MSG_CHECKING(whether C compiler accepts $1)
	OLD_CPPFLAGS="$CPPFLAGS"
	if test "x$ac_compiler_gnu" = "xyes" ; then
		CPPFLAGS="$CPPFLAGS -Werror $1"
	else
		CPPFLAGS="$CPPFLAGS $1"
	fi
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
		[ 
			CPPFLAGS="$OLD_CPPFLAGS $1"
			AC_MSG_RESULT(yes) 
		], 
		[ 
			CPPFLAGS="$OLD_CPPFLAGS"
			AC_MSG_RESULT(no)
		])
])

AC_DEFUN([AC_CXX_CHECK_OPTION],[
	AC_MSG_CHECKING(whether C++ compiler accepts $1)
	OLD_CXXFLAGS="$CXXFLAGS"
	if test "x$ac_compiler_gnu" = "xyes" ; then
		CXXFLAGS="$CXXFLAGS -Werror $1"
	else
		CXXFLAGS="$CXXFLAGS $1"
	fi
	AC_LANG_PUSH([C++])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
		[ 
			CXXFLAGS="$OLD_CXXFLAGS $1"
			AC_MSG_RESULT(yes) 
		], 
		[ 
			CXXFLAGS="$OLD_CXXFLAGS"
			AC_MSG_RESULT(no)
		])
	AC_LANG_POP([C++])
])

AC_DEFUN([AC_LD_CHECK_OPTION],[
	AC_MSG_CHECKING(whether linker accepts $1)
	OLD_LDFLAGS="$LDFLAGS"
	if test "x$ac_compiler_gnu" = "xyes" ; then
		LDFLAGS="$LDFLAGS -Werror $1"
	else
		LDFLAGS="$LDFLAGS $1"
	fi
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
		[
			LDFLAGS="$OLD_LDFLAGS $1"
			AC_MSG_RESULT(yes)
		],
		[
			LDFLAGS="$OLD_LDFLAGS"
			AC_MSG_RESULT(no)
		])
])


#--------------------------------------------------------------------
# Initialize variables
#--------------------------------------------------------------------

PLATFORM=`uname -s`
CPUTYPE=`uname -p`
COMPONENTS=""
DISABLE_ICONV="no"
DISABLE_ENCRYPTION="no"
DISABLE_LMSENSORS="no"
REQUIRE_LMSENSORS="no"
DISABLE_SSH="no"
REQUIRE_SSH="no"
DISABLE_JQ="no"
REQUIRE_JQ="no"
ENABLE_DEBUG="no"
ENABLE_SANITIZER="no"
ENABLE_WERROR="no"
ENABLE_RTTI="no"
STATIC_BUILD="no"
ALL_STATIC="no"
DBDRIVER_SELECTED="no"
USE_PTH="no"
BUILD_SERVER="no"
BUILD_AGENT="no"
BUILD_CLIENT="no"
BUILD_GUI_CLIENT="yes"
BUILD_STATIC_AGENT="no"
BUILD_SDK="no"
MODULES="libnetxms install"
TEST_MODULES=""
AGENT_UNIT_TESTS=""
TOOLS=""
STATIC_SUBAGENT_LIST=""
PROPOSED_STATIC_SUBAGENTS="default"
SUBAGENT_DIRS=""
SUBAGENT_LIBS=""
AGENT_LIBS=""
AGENT_DIRS=""
SERVER_LIBS=""
SERVER_TOOLS=""
SERVER_INCLUDE=""
NXADM_LIBS=""
NCDRV_MODULES=""
NCDRV_LTLIBRARIES=""
HDLINK_DIRS=""
PDSDRV_DIRS=""
TOP_LEVEL_MODULES=""
CONTRIB_MODULES=""
CLIENT_COMPONENTS=""
BUILD_UNICODE="yes"
FORCE_INTERNAL_JANSSON="no"
FORCE_INTERNAL_SQLITE="no"
FORCE_32BIT_BUILD="no"
FORCE_INTERNAL_GETOPT="no"
LDFLAGS_PREFIX=""
LDFLAGS_SUFFIX=""
LTINIT_CC=""
LTINIT_CXX=""
SQLFLAGS=""
RUNTIME_PREFIX=""
RUNTIME_STATE_DIR=""
XMPP_SUPPORT="no"
DISABLE_XMPP="no"
REQUIRE_XMPP="no"
DISABLE_ZEROMQ="no"
REQUIRE_ZEROMQ="no"
ZEROMQ_SUPPORT="no"
ZEROMQ_CPPFLAGS=""
ZEROMQ_LDFLAGS=""
MQTT_SUPPORT="no"
DISABLE_MQTT="no"
REQUIRE_MQTT="no"
MQTT_CPPFLAGS=""
MQTT_LDFLAGS=""
MQTT_LIBS=""
PYTHON_SUPPORT="no"
PYTHON_CPPFLAGS=""
PYTHON_LDFLAGS=""
PYTHON_LIBS=""
LDAP_SUPPORT="no"
DISABLE_LDAP="no"
REQUIRE_LDAP="no"
DISABLE_MODBUS="no"
REQUIRE_MODBUS="no"
DISABLE_ISOTREE="no"
REQUIRE_ISOTREE="no"
DISABLE_READLINE="yes"
TUXEDO_LIBS=""
TUXEDO_TPINIT=""
MONGODB_LIBS=""
ASTERISK_LIBS=""
VMGR_CPPFLAGS=""
VMGR_LDFLAGS=""
VMGR_LIBS=""
LIBVIRT_BASE=""
XEN_CPPFLAGS=""
XEN_LDFLAGS=""
XEN_LIBS=""
DISABLE_IPV6="no"
IPV6_SUPPORT="no"
INSTALL_LIBPATH=""
WITH_ENCRYPTION="no"
OPENSSL_LIBSSL="ssl"
OPENSSL_LIBCRYPTO="crypto"
SYSTEMD_SUPPORT="yes"
HAVE_SDBUS="no"
EXEC_LDFLAGS=""
EXEC_LIBS=""
DIST_BUILD="no"
FORCE_NULL_SAFE_FREE="no"
JEMALLOC_REQUIRED="no"
MARIADB_MYSQL_CONFLICT="no"
FORCE_INTERNAL_RWLOCK="no"
LIBNXAGENT_LIBS=""
LIBNXSRV_LIBS=""
JAR_BUILD="yes"
RELEASE_BUILD="no"
SYSTEMD_LIBS=""
REQUIRE_PROTOBUF="no"
REQUIRE_MICROHTTPD="no"
REQUIRE_SNAPPY="no"


#--------------------------------------------------------------------
# Common compilation flags
#--------------------------------------------------------------------

CPPFLAGS="$CPPFLAGS -D_THREAD_SAFE"


#--------------------------------------------------------------------
# Parse command line parameters
#--------------------------------------------------------------------

AC_ARG_WITH(runtime-prefix,
[AS_HELP_STRING(--with-runtime-prefix=prefix,installation prefix to be used at run time)],
[
   RUNTIME_PREFIX="$withval"
   RUNTIME_STATE_DIR="$RUNTIME_PREFIX/var"
])

AC_ARG_WITH(runtime-state-dir,
[AS_HELP_STRING(--with-runtime-state-dir=path,state directory to be used at run time)],
[
   RUNTIME_STATE_DIR="$withval"
])

AC_ARG_WITH(clang,
[AS_HELP_STRING(--with-clang,use clang C++ compiler on Linux if available)],
[
	ALLOW_CLANG="yes"
])

AC_ARG_WITH(server,
[AS_HELP_STRING(--with-server,build server)],
[
	COMPONENTS="$COMPONENTS sqlite ethernetip snmp server agent sqlite_drv"
])

AC_ARG_WITH(snmp,
[AS_HELP_STRING(--with-snmp,build SNMP library and tools)],
[
	COMPONENTS="$COMPONENTS snmp"
])

AC_ARG_WITH(flow-collector,
[AS_HELP_STRING(--with-flow-collector,build NetFlow/IPFIX flow collector)],
[
	COMPONENTS="$COMPONENTS flow-collector"
])

AC_ARG_WITH(client,
[AS_HELP_STRING(--with-client,build client library and tools)],
[
	COMPONENTS="$COMPONENTS client"
])

AC_ARG_WITH(gui-client,
[AS_HELP_STRING(--without-gui-client,disable build of GUI client)],
[
	if test "x$withval" = "xno" ; then
		BUILD_GUI_CLIENT="no"
	fi
])

AC_ARG_WITH(client-proxy,
[AS_HELP_STRING(--with-client-proxy,build client proxy)],
[
	COMPONENTS="$COMPONENTS client-proxy"
])

AC_ARG_WITH(agent,
[AS_HELP_STRING(--with-agent,build agent)],
[
	COMPONENTS="$COMPONENTS sqlite ethernetip snmp agent sqlite_drv"
])

AC_ARG_WITH(prometheus,
[AS_HELP_STRING(--with-prometheus,build Prometheus remote write support for agent)],
[
	COMPONENTS="$COMPONENTS prometheus"
	REQUIRE_MICROHTTPD="yes"
	REQUIRE_PROTOBUF="yes"
	REQUIRE_SNAPPY="yes"
])

AC_ARG_WITH(static-agent,
[AS_HELP_STRING(--with-static-agent,build statically linked agent)],
[
	COMPONENTS="$COMPONENTS sqlite snmp static-agent sqlite_drv"
	FORCE_INTERNAL_JANSSON="yes"
	FORCE_INTERNAL_SQLITE="yes"
])

AC_ARG_WITH(sdk,
[AS_HELP_STRING(--with-sdk,build and install files necessary for extension development)],
[
	COMPONENTS="$COMPONENTS sqlite sdk"
	TOP_LEVEL_MODULES="$TOP_LEVEL_MODULES include"
	SERVER_INCLUDE="include"
	BUILD_SDK="yes"
])

AC_ARG_WITH(static-sdk,
[AS_HELP_STRING(--with-static-sdk,build and install files necessary for extension development (static libs only))],
[
	COMPONENTS="$COMPONENTS sqlite sdk"
	TOP_LEVEL_MODULES="$TOP_LEVEL_MODULES include"
	SERVER_INCLUDE="include"
	BUILD_SDK="yes"
	STATIC_BUILD="yes"
])

AC_ARG_WITH(mdebug,
[AS_HELP_STRING(--with-mdebug,use debug version of memory allocation functions)],
[
	CPPFLAGS="$CPPFLAGS -DNETXMS_MEMORY_DEBUG"
])

AC_ARG_WITH(profiler,
[AS_HELP_STRING(--with-profiler,link with libprofiler from gperftools)],
[
	LDFLAGS="$LDFLAGS -lprofiler"
])

AC_ARG_WITH(private-extensions,
[AS_HELP_STRING(--with-private-extensions,enable private extensions)],
[
	if test "x$withval" != "xno" ; then
		WITH_PRIVATE_EXTENSIONS="yes"
	        if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
			CUSTOM_INCLUDE="$withval"
		fi
		AC_DEFINE(WITH_PRIVATE_EXTENSIONS, 1, Define to 1 if private extensions are enabled)
	fi
])

AC_ARG_WITH(zlib,
[AS_HELP_STRING(--with-zlib,specify zlib location)],
[
	if test "x$withval" != "x" && test "x$withval" != "xyes" && test "x$withval" != "xno"; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
		LDFLAGS="$LDFLAGS -L${withval}/lib"
		CPPFLAGS="$CPPFLAGS -I${withval}/include"
	fi
])

AC_ARG_WITH(libexpat,
[AS_HELP_STRING(--with-libexpat,specify libexpat location)],
[
	if test "x$withval" != "x" && test "x$withval" != "xyes" && test "x$withval" != "xno"; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
		LDFLAGS="$LDFLAGS -L${withval}/lib"
		CPPFLAGS="$CPPFLAGS -I${withval}/include"
	fi
])

AC_ARG_WITH(internal-libjansson,
[AS_HELP_STRING(--with-internal-libjansson,force use of bundled libjansson)],
[
        FORCE_INTERNAL_JANSSON="yes"
])

AC_ARG_WITH(internal-sqlite,
[AS_HELP_STRING(--with-internal-sqlite,force use of bundled sqlite)],
[
	FORCE_INTERNAL_SQLITE="yes"
])

AC_ARG_WITH(internal-getopt,
[AS_HELP_STRING(--with-internal-getopt,force use of bundled getopt)],
[
	FORCE_INTERNAL_GETOPT="yes"
])

AC_ARG_WITH(internal-rwlock,
[AS_HELP_STRING(--with-internal-rwlock,force use of bundled read/write lock implementation)],
[
	FORCE_INTERNAL_RWLOCK="yes"
])

AC_ARG_WITH(all-static,
[AS_HELP_STRING(--with-all-static,use -all-static flag for linking executables)],
[
	ALL_STATIC="yes"
])

AC_ARG_WITH(static-subagents,
[AS_HELP_STRING(--with-static-subagents-list,define list of static subagents (separated by spaces))],
[
	PROPOSED_STATIC_SUBAGENTS="$withval"
])

AC_ARG_WITH(jdk,
[AS_HELP_STRING(--with-jdk,build Java-dependent components)],
[
   if test "x$withval" != "xno" ; then
	   if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		   JDK_LOCATION="$withval"
		   AC_DEFINE_UNQUOTED([JDK_LOCATION],["$withval"],[JDK location])
	   fi
	   COMPONENTS="$COMPONENTS java"
   fi
])

AC_ARG_WITH(tuxedo,
[AS_HELP_STRING(--with-tuxedo,build Tuxedo-dependent components)],
[
   if test "x$withval" != "xno" ; then
	   if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		   TUXEDO_BASE="$withval"
	   fi
	   COMPONENTS="$COMPONENTS tuxedo"
fi
])

AC_ARG_WITH(zeromq,
[AS_HELP_STRING(--with-zeromq,build ZeroMQ-dependent components)],
[
   if test "x$withval" != "xno" ; then
	   REQUIRE_ZEROMQ=yes
	   if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		   ZEROMQ_BASE="$withval"
	   fi
   else
	   DISABLE_ZEROMQ=yes
	   REQUIRE_ZEROMQ=no
   fi
])

AC_ARG_WITH(mosquitto,
[AS_HELP_STRING(--with-mosquitto,specify mosquitto library location)],
[
   if test "x$withval" != "xno" ; then
	   if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		   MOSQUITTO_BASE="$withval"
	   fi
   fi
])

AC_ARG_ENABLE(mqtt,
[AS_HELP_STRING(--disable-mqtt,disable MQTT support)],
[
	if test "x$enableval" = "xno"; then
		DISABLE_MQTT="yes"
	fi
	if test "x$enableval" = "xyes"; then
		DISABLE_MQTT="no"
		REQUIRE_MQTT="yes"
	fi
])

AC_ARG_WITH(curl,
[AS_HELP_STRING(--with-curl,specify libcurl location)],
[
	if test "x$withval" = "xno"; then
		AC_MSG_ERROR(libcurl is required)
	else
		if test "x$withval" != "xyes"; then
			CPPFLAGS="$CPPFLAGS -I$withval/include"
			LDFLAGS="$LDFLAGS -L$withval/lib"
			LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
		fi
	fi
])

AC_ARG_WITH(python,
[AS_HELP_STRING(--with-python,build with Python extensions support)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		PYTHON_BASE="$withval"
		PYTHON_REQUIRED=yes
	fi
	COMPONENTS="$COMPONENTS python"
	PYTHON_SUPPORT=yes
else
	PYTHON_SUPPORT=no
fi ])

AC_ARG_WITH(mongodb,
[AS_HELP_STRING(--with-mongodb,build MongoDB-dependent components)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		MONGODB_BASE="$withval"
	fi
	COMPONENTS="$COMPONENTS mongodb"
fi ])

AC_ARG_WITH(redis,
[AS_HELP_STRING(--with-redis,build Redis monitoring subagent)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		REDIS_CPPFLAGS="-I${withval}/include"
		REDIS_LDFLAGS="-L${withval}/lib"
	fi
	COMPONENTS="$COMPONENTS redis"
fi ])

AC_ARG_WITH(asterisk,
[AS_HELP_STRING(--with-asterisk,build Asterisk subagent)],
[ if test "x$withval" != "xno" ; then
	COMPONENTS="$COMPONENTS asterisk"
fi ])

AC_ARG_WITH(vmgr,
[AS_HELP_STRING(--with-vmgr,build VirtualHost management subagent)],
[ if test "x$withval" != "xno" ; then
	COMPONENTS="$COMPONENTS vmgr"
fi ])

AC_ARG_WITH(libvirt,
[AS_HELP_STRING(--with-libvirt,provide libvirt location)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		LIBVIRT_BASE="$withval"
	fi
fi ])

AC_ARG_WITH(xen,
[AS_HELP_STRING(--with-xen,build XEN subagent)],
[ if test "x$withval" != "xno" ; then
	COMPONENTS="$COMPONENTS xen"
fi ])

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
		ODBC_LD_RUN_PATH="${withval}/lib"   
		ODBC_LDFLAGS="-L${withval}/lib"
		ODBC_CPPFLAGS="-I${withval}/include"
	fi
	COMPONENTS="$COMPONENTS odbc"
	DBDRIVER_SELECTED="yes"
fi ])

AC_ARG_WITH(oracle,
[AS_HELP_STRING(--with-oracle,build Oracle database driver)],
[ if test "x$withval" != "xno" ; then
	ORABASE="$ORACLE_HOME"
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		ORABASE="$withval"
	fi
	if test "x$ORABASE" != "x"; then
		if test -x "$ORABASE/lib" ; then
			ORACLE_LD_RUN_PATH="${ORABASE}/lib"   
			ORACLE_LDFLAGS="-L${ORABASE}/lib"
		else
			ORACLE_LD_RUN_PATH="${ORABASE}"   
			ORACLE_LDFLAGS="-L${ORABASE}"
		fi
		if test -x "$ORABASE/include" ; then
			ORACLE_CPPFLAGS="-I${ORABASE}/include"
		fi
		if test -x "$ORABASE/sdk/include" ; then
			ORACLE_CPPFLAGS="-I${ORABASE}/sdk/include"
		fi
		if test -x "$ORABASE/rdbms/public" ; then
			ORACLE_CPPFLAGS="-I${ORABASE}/rdbms/public"
		fi
	fi
	COMPONENTS="$COMPONENTS oracle"
	DBDRIVER_SELECTED="yes"
fi ])

AC_ARG_WITH(mysql,
[AS_HELP_STRING(--with-mysql,build MySQL database driver)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		if find_existing_dir "${withval}/lib" "${withval}/lib/mysql" "${withval}/mysql/lib"; then
			MYSQL_LD_RUN_PATH="$__dir$"    
			MYSQL_LDFLAGS="-L$__dir"
		fi
		if find_existing_dir "${withval}/include" "${withval}/include/mysql" "${withval}/mysql/include"; then
			MYSQL_CPPFLAGS="-I$__dir"
		fi
	else
		if find_existing_dir "/usr/local/mysql/lib" "/usr/local/mysql/lib/mysql" "/usr/lib64/mysql" "/usr/lib/mysql" "/usr/mysql/lib" "/usr/mysql/lib/mysql" "/usr/local/lib/mysql"; then
			MYSQL_LD_RUN_PATH="$__dir$"    
			MYSQL_LDFLAGS="-L$__dir"
		fi
		if find_existing_dir "/usr/local/mysql/include" "/usr/include/mysql" "/usr/mysql/include" "/usr/local/include/mysql" "/usr/local/mysql/include/mysql" "/usr/mysql/include/mysql"; then
			MYSQL_CPPFLAGS="-I$__dir"
		fi
	fi
	COMPONENTS="$COMPONENTS mysql"
	DBDRIVER_SELECTED="yes"
fi ])

AC_ARG_WITH(mariadb,
[AS_HELP_STRING(--with-mariadb,build MariaDB database driver)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		if find_existing_dir "${withval}/lib" "${withval}/lib/mariadb" "${withval}/mariadb/lib"; then
			MARIADB_LD_RUN_PATH="$__dir$"    
			MARIADB_LDFLAGS="-L$__dir"
		fi
		if find_existing_dir "${withval}/include" "${withval}/include/mariadb" "${withval}/mariadb/include"; then
			MARIADB_CPPFLAGS="-I$__dir"
		fi
	else
		if find_existing_dir "/usr/local/mariadb/lib" "/usr/local/lib/mariadb"; then
			MARIADB_LD_RUN_PATH="$__dir$"    
			MARIADB_LDFLAGS="-L$__dir"
		fi
		if find_existing_dir "/usr/local/mariadb/include" "/usr/include/mariadb" "/usr/local/include/mariadb"; then
			MARIADB_CPPFLAGS="-I$__dir"
		fi
	fi
	COMPONENTS="$COMPONENTS mariadb"
	DBDRIVER_SELECTED="yes"
fi ])

AC_ARG_WITH(mariadb-compat-headers,
[AS_HELP_STRING(--with-mariadb-compat-headers,MariaDB client library installed as MySQL client replacement)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		if find_existing_dir "${withval}/include" "${withval}/include/mysql" "${withval}/mysql/include"; then
			MARIADB_CPPFLAGS="-I$__dir"
		fi
	else
		if find_existing_dir "/usr/include/mysql" "/usr/local/include/mysql" "/usr/local/mysql/include/mysql"; then
			MARIADB_CPPFLAGS="-I$__dir"
		fi
	fi
	MARIADB_MYSQL_CONFLICT="yes"
fi ])

AC_ARG_WITH(pgsql,
[AS_HELP_STRING(--with-pgsql,build PostgreSQL database driver)],
[ if test "x$withval" != "xno" ; then
	if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
		if find_existing_dir "${withval}/lib/postgresql" "${withval}/lib/postgresql@14" "${withval}/lib" "${withval}/lib/pgsql" "${withval}/pgsql/lib" "${withval}/postgresql/lib"; then
			PGSQL_LD_RUN_PATH="$__dir$"    
			PGSQL_LDFLAGS="-L$__dir"
		fi
		if find_existing_dir "${withval}/include/postgresql" "${withval}/include/postgresql@14" "${withval}/include" "${withval}/include/pgsql" "${withval}/pgsql/include" "${withval}/postgresql/include"; then
			PGSQL_CPPFLAGS="-I$__dir"
		fi
	else
		if find_existing_dir "/usr/local/pgsql/lib" "/usr/local/pgsql/lib/pgsql" "/usr/lib64/pgsql" "/usr/lib/pgsql" "/usr/pgsql/lib" "/usr/pgsql/lib/pgsql" "/usr/local/lib/pgsql" "/usr/local/postgresql/lib" "/usr/local/postgresql/lib/postgresql" "/usr/lib/postgresql" "/usr/postgresql/lib" "/usr/postgresql/lib/postgresql" "/usr/local/lib/postgresql"; then
			PGSQL_LD_RUN_PATH="$__dir$"    
			PGSQL_LDFLAGS="-L$__dir"
		fi
		if find_existing_dir "/usr/local/pgsql/include" "/usr/include/pgsql" "/usr/pgsql/include" "/usr/local/include/pgsql" "/usr/local/pgsql/include/pgsql" "/usr/pgsql/include/pgsql" "/usr/local/postgresql/include" "/usr/include/postgresql" "/usr/postgresql/include" "/usr/local/include/postgresql" "/usr/local/postgresql/include/postgresql" "/usr/postgresql/include/postgresql"; then
			PGSQL_CPPFLAGS="-I$__dir"
		fi
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
			DB2_LD_RUN_PATH="${withval}/${DB2_LIB_DIR}"   
			DB2_LDFLAGS="-L${withval}/${DB2_LIB_DIR}"
		else
			DB2_LIB_DIR=lib
			if test -x "$withval/${DB2_LIB_DIR}" ; then
				DB2_LD_RUN_PATH="${withval}/${DB2_LIB_DIR}"   
				DB2_LDFLAGS="-L${withval}/${DB2_LIB_DIR}"
			else
				DB2_LD_RUN_PATH="${withval}"   
				DB2_LDFLAGS="-L${withval}"
			fi
		fi
		if test -x "$withval/include" ; then
			DB2_CPPFLAGS="-I${withval}/include"
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
			INFORMIX_LD_RUN_PATH="${withval}/lib/cli"   
			INFORMIX_LDFLAGS="-L${withval}/lib/cli"
			if test -x "$withval/lib/esql" ; then
				INFORMIX_LD_RUN_PATH="${withval}/lib/esql:${INFORMIX_LD_RUN_PATH}"   
				INFORMIX_LDFLAGS="$INFORMIX_LDFLAGS -L${withval}/lib/esql"
			fi
		else
			INFORMIX_LD_RUN_PATH="${withval}"   
			INFORMIX_LDFLAGS="-L${withval}"
		fi
		if test -x "$withval/incl/cli" ; then
			INFORMIX_CPPFLAGS="-I${withval}/incl/cli"
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
])

AC_ARG_WITH(pcre,
[AS_HELP_STRING(--with-pcre,specify PCRE location)],
[
	if test "x$withval" != "x" && test "x$withval" != "xyes" && test "x$withval" != "xno"; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"
		LDFLAGS="$LDFLAGS -L${withval}/lib"
		CPPFLAGS="$CPPFLAGS -I${withval}/include"
	fi
])

AC_ARG_WITH(pcre2,
[AS_HELP_STRING(--with-pcre2,specify PCRE2 location)],
[
	if test "x$withval" != "x" && test "x$withval" != "xyes" && test "x$withval" != "xno"; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"
		LDFLAGS="$LDFLAGS -L${withval}/lib"
		CPPFLAGS="$CPPFLAGS -I${withval}/include"
	fi
])

AC_ARG_WITH(bundled-openssl,
[AS_HELP_STRING(--with-bundled-openssl,use bundled OpenSSL libraries with product-specific prefix)],
[
	OPENSSL_LIBSSL="nxssl"
	OPENSSL_LIBCRYPTO="nxcrypto"
])

AC_ARG_ENABLE(ssh,
[AS_HELP_STRING(--disable-ssh,disable SSH support)],
[
	if test "x$enableval" = "xno"; then
		DISABLE_SSH="yes"
	fi
	if test "x$enableval" = "xyes"; then
		DISABLE_SSH="no"
		REQUIRE_SSH="yes"
	fi
])

AC_ARG_WITH(libssh,
[AS_HELP_STRING(--with-libssh,specify libssh location)],
[
	if test "x$withval" != "x" && test "x$withval" != "xyes" && test "x$withval" != "xno" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"
		LIBSSH_LDFLAGS="-L${withval}/lib"
		LIBSSH_CPPFLAGS="-I${withval}/include"
	fi
])

AC_ARG_ENABLE(jq,
[AS_HELP_STRING(--disable-jq,disable jq support)],
[
	if test "x$enableval" = "xno"; then
		DISABLE_JQ="yes"
	fi
	if test "x$enableval" = "xyes"; then
		DISABLE_JQ="no"
		REQUIRE_JQ="yes"
	fi
])

AC_ARG_WITH(libjq,
[AS_HELP_STRING(--with-libjq,specify libjq location)],
[
	if test "x$withval" != "x" && test "x$withval" != "xyes" && test "x$withval" != "xno" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"
		LIBJQ_LDFLAGS="-L${withval}/lib"
		LIBJQ_CPPFLAGS="-I${withval}/include"
	fi
])

AC_ARG_WITH(microhttpd,
[AS_HELP_STRING(--with-microhttpd,specify libmicrohttpd location)],
[
	if test "x$withval" != "x" && test "x$withval" != "xyes" && test "x$withval" != "xno" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"
		MICROHTTPD_LDFLAGS="$LDFLAGS -L${withval}/lib"
		MICROHTTPD_CPPFLAGS="$CPPFLAGS -I${withval}/include"
	fi
])

AC_ARG_WITH(abseil,
[AS_HELP_STRING(--with-abseil,specify abseil-cpp location)],
[
	if test "x$withval" != "x" && test "x$withval" != "xyes" && test "x$withval" != "xno" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"
		ABSEIL_LDFLAGS="$LDFLAGS -L${withval}/lib"
		ABSEIL_CPPFLAGS="$CPPFLAGS -I${withval}/include"
	fi
])

AC_ARG_WITH(protobuf,
[AS_HELP_STRING(--with-protobuf,specify protobuf location)],
[
	if test "x$withval" != "x" && test "x$withval" != "xyes" && test "x$withval" != "xno" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"
		PROTOBUF_LDFLAGS="$LDFLAGS -L${withval}/lib"
		PROTOBUF_CPPFLAGS="$CPPFLAGS -I${withval}/include"
	fi
])

AC_ARG_WITH(snappy,
[AS_HELP_STRING(--with-snappy,specify libsnappy location)],
[
	if test "x$withval" != "x" && test "x$withval" != "xyes" && test "x$withval" != "xno" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"
		SNAPPY_LDFLAGS="$LDFLAGS -L${withval}/lib"
		SNAPPY_CPPFLAGS="$CPPFLAGS -I${withval}/include"
	fi
])

AC_ARG_WITH(jemalloc,
[AS_HELP_STRING(--with-jemalloc,specify libjemalloc location)],
[
	if test "x$withval" != "xno" ; then
		JEMALLOC_REQUIRED="yes"
		if test "x$withval" != "x" && test "x$withval" != "xyes" ; then
			LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
			LDFLAGS="$LDFLAGS -L${withval}/lib"
			CPPFLAGS="$CPPFLAGS -I${withval}/include"
			INSTALL_LIBPATH="${withval}/lib${INSTALL_LIBPATH:+:}${INSTALL_LIBPATH}"
		fi
	fi
])

AC_ARG_ENABLE(iconv,
[AS_HELP_STRING(--disable-iconv,do not use iconv() for text conversions)],
[
	if test "x$enableval" = "xno"; then
		DISABLE_ICONV="yes"
	fi
])

AC_ARG_ENABLE(ipv6,
[AS_HELP_STRING(--disable-ipv6,disable IPv6 support)],
[
	if test "x$enableval" = "xno"; then
		DISABLE_IPV6="yes"
	fi
])

AC_ARG_ENABLE(encryption,
[AS_HELP_STRING(--disable-encryption,disable encryption support)],
[
	if test "x$enableval" = "xno"; then
		DISABLE_ENCRYPTION="yes"
		DISABLE_XMPP="yes"
	fi
])

AC_ARG_ENABLE(lmsensors,
[AS_HELP_STRING(--disable-lmsensors,disable lmsensors support)],
[
	if test "x$enableval" = "xno"; then
		DISABLE_LMSENSORS="yes"
	fi
	if test "x$enableval" = "xyes"; then
		DISABLE_LMSENSORS="no"
		REQUIRE_LMSENSORS="yes"
	fi
])

AC_ARG_ENABLE(xmpp,
[AS_HELP_STRING(--disable-xmpp,disable XMPP support)],
[
	if test "x$enableval" = "xno"; then
		DISABLE_XMPP="yes"
	fi
	if test "x$enableval" = "xyes"; then
		DISABLE_XMPP="no"
		REQUIRE_XMPP="yes"
	fi
])

AC_ARG_WITH(rpi-agent,
[AS_HELP_STRING(--with-rpi-agent,build Raspberry Pi agent)],
[
	COMPONENTS="$COMPONENTS raspberrypi"
])

AC_ARG_WITH(openwrt,
[AS_HELP_STRING(--with-openwrt,build for OpenWrt platform)],
[
	AC_DEFINE(_OPENWRT, 1, Define to 1 when building for OpenWrt platform)
])

AC_ARG_ENABLE(ldap,
[AS_HELP_STRING(--disable-ldap,disable LDAP support)],
[
	if test "x$enableval" = "xno"; then
		DISABLE_LDAP="yes"
	fi
	if test "x$enableval" = "xyes"; then
		DISABLE_LDAP="no"
		REQUIRE_LDAP="yes"
	fi
])

AC_ARG_ENABLE(modbus,
[AS_HELP_STRING(--disable-modbus,disable MODBUS support)],
[
	if test "x$enableval" = "xno"; then
		DISABLE_MODBUS="yes"
	fi
	if test "x$enableval" = "xyes"; then
		DISABLE_MODBUS="no"
		REQUIRE_MODBUS="yes"
	fi
])

AC_ARG_WITH(libmodbus,
[AS_HELP_STRING(--with-libmodbus,specify libmodbus location)],
[
	if test "x$withval" != "x" && test "x$withval" != "xyes" && test "x$withval" != "xno" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"
		LIBMODBUS_LDFLAGS="-L${withval}/lib"
		LIBMODBUS_CPPFLAGS="-I${withval}/include"
	fi
])

AC_ARG_ENABLE(isotree,
[AS_HELP_STRING(--disable-isotree,disable isolation tree algorithm)],
[
	if test "x$enableval" = "xno"; then
		DISABLE_ISOTREE="yes"
	fi
	if test "x$enableval" = "xyes"; then
		DISABLE_ISOTREE="no"
		REQUIRE_ISOTREE="yes"
	fi
])

AC_ARG_WITH(libisotree,
[AS_HELP_STRING(--with-libisotree,specify libisotree location)],
[
	if test "x$withval" != "x" && test "x$withval" != "xyes" && test "x$withval" != "xno" ; then
		LD_RUN_PATH="${withval}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"
		LIBISOTREE_LDFLAGS="-L${withval}/lib"
		LIBISOTREE_CPPFLAGS="-I${withval}/include"
	fi
])

AC_ARG_ENABLE(unicode,
[AS_HELP_STRING(--disable-unicode,disable UNICODE build)],
[
	if test "x$enableval" = "xno"; then
		BUILD_UNICODE="no"
		DISABLE_READLINE="no"
	fi
])

AC_ARG_ENABLE(visibility-control,
[AS_HELP_STRING(--disable-visibility-control,disable symbol visibility control)],
[
	if test "x$enableval" = "xno"; then
		DISABLE_VISIBILITY_CONTROL="yes"
	fi
])

AC_ARG_ENABLE(debug,
[AS_HELP_STRING(--enable-debug,enable additional debugging functionality)],
[
	if test "x$enableval" = "xyes"; then
		ENABLE_DEBUG="yes"
	fi
])

AC_ARG_ENABLE(sanitizer,
[AS_HELP_STRING(--enable-sanitizer,turn on GCC sanitizer functionality)],
[
	if test "x$enableval" = "xyes"; then
		ENABLE_SANITIZER="yes"
	fi
])

AC_ARG_ENABLE(werror,
[AS_HELP_STRING(--enable-werror,threat all warnings as errors)],
[
	if test "x$enableval" = "xyes"; then
		ENABLE_WERROR="yes"
	fi
])

AC_ARG_ENABLE(rtti,
[AS_HELP_STRING(--enable-rtti,enable RTTI)],
[
	if test "x$enableval" = "xyes"; then
		ENABLE_RTTI="yes"
	fi
])

AC_ARG_ENABLE(64bit,
[AS_HELP_STRING(--disable-64bit,disable 64bit build (force 32bit build on 64bit machine))],
[
	if test "x$enableval" = "xno"; then
		FORCE_32BIT_BUILD="yes"
	fi
])

AC_ARG_WITH(dist,
[AS_HELP_STRING(--with-dist,for maintainers only)],
	DB_DRIVERS="mysql mariadb pgsql odbc mssql sqlite oracle db2 informix"
	MODULES="appagent jansson java-common libnetxms libnxjava install sqlite snmp ethernetip flow-collector libnxsl libnxmb libnxlp libnxpython libnxcc db client server ncdrivers agent nxscript nxcproxy mobile-agent"
	TEST_MODULES="agent test-libnxcc test-libnxsl test-libnxsnmp"
	AGENT_UNIT_TESTS="linux-cpu-usage-collector"
	TOOLS="nxlptest"
	SUBAGENT_DIRS="linux ds18x20 freebsd openbsd minix mqtt mysql pgsql netbsd sunos aix informix oracle prometheus lmsensors darwin rpi java jmx opcua ubntlw db2 tuxedo mongodb ssh vmgr xen asterisk python redis"
	AGENT_DIRS="libnxappc libnxtux"
	NCDRV_MODULES="anysms googlechat kannel mattermost mqtt msteams mymobile nexmo nxagent slack smtp smseagle telegram text2reach twilio websms xmpp"
	HDLINK_DIRS="jira redmine"
	PDSDRV_DIRS="clickhouse influxdb rrdtool"
	TOP_LEVEL_MODULES="include sql images tests"
	SERVER_INCLUDE="include"
	CONTRIB_MODULES="mibs backgrounds music oui templates"
	CLIENT_COMPONENTS="java nxalarm nxevent nxmc nxnotify nxpush nxshell"
	SERVER_COMPONENTS="nxreportd webapi"
	DIST_BUILD="yes"
)

AC_ARG_WITH(tests,
[AS_HELP_STRING(--with-tests,build tests)],
[
	TOP_LEVEL_MODULES="$TOP_LEVEL_MODULES tests"
])

AC_ARG_ENABLE(systemd,
[AS_HELP_STRING(--disable-systemd,disable Systemd support)],
[
	if test "x$enableval" = "xno"; then
		SYSTEMD_SUPPORT="no"
	fi
])

AC_ARG_ENABLE(null-free-check,
[AS_HELP_STRING(--disable-null-free-check,[disable NULL checking in free() calls (only use for cross-compiling)])],
[
	if test "x$enableval" = "xno"; then
		FORCE_NULL_SAFE_FREE="yes"
	fi
])

AC_ARG_ENABLE(jar-build,
	      [AS_HELP_STRING(--disable-jar-build,disable build and installation of jar files)],
[
	JAR_BUILD="no"
])

AC_ARG_ENABLE(release-build,
[AS_HELP_STRING(--enable-release-build,enable release build)],
[
	if test "x$enableval" = "xyes"; then
		RELEASE_BUILD="yes"
	fi
])


#--------------------------------------------------------------------
# Set LD_LIBRARY_PATH from LD_RUN_PATH because on some systems
# LD_RUN_PATH ignored by linker and some tests may fail
#--------------------------------------------------------------------

export LD_RUN_PATH
if test "x$LD_RUN_PATH" != "x"; then
	LD_LIBRARY_PATH="${LD_LIBRARY_PATH}${LD_LIBRARY_PATH:+:}${LD_RUN_PATH}"
	export LD_LIBRARY_PATH
fi


#--------------------------------------------------------------------
# Validate command line parameters and set various checking options
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "snmp"
if test $? = 0; then
	MODULES="$MODULES snmp"
	TEST_MODULES="$TEST_MODULES test-libnxsnmp"
fi

check_substr "$COMPONENTS" "ethernetip"
if test $? = 0; then
	MODULES="$MODULES ethernetip"
fi

check_substr "$COMPONENTS" "flow-collector"
if test $? = 0; then
	MODULES="$MODULES flow-collector"
fi

check_substr "$COMPONENTS" "java"
if test $? = 0; then
	MODULES="$MODULES java-common libnxjava"
	if test "x$JAR_BUILD" = "xyes"; then
		SUBAGENT_DIRS="$SUBAGENT_DIRS java jmx opcua ubntlw"
	else
		SUBAGENT_DIRS="$SUBAGENT_DIRS java"
	fi
fi

check_substr "$COMPONENTS" "python"
if test $? = 0; then
	MODULES="$MODULES libnxpython"
	SUBAGENT_DIRS="$SUBAGENT_DIRS python"
fi

check_substr "$COMPONENTS" "static-agent"
if test $? = 0; then
	if test "x$COMPONENTS" != "x sqlite snmp static-agent sqlite_drv"; then
		AC_MSG_ERROR(Static agent and other components are mutually exclusive.)
	fi
	STATIC_BUILD="yes"
	BUILD_STATIC_AGENT="yes"
	MODULES="$MODULES appagent libnxlp db agent"
	DISABLE_ICONV="yes"
	CPPFLAGS="$CPPFLAGS -D_STATIC_AGENT"
	if test "x$PROPOSED_STATIC_SUBAGENTS" = "xdefault"; then
		STATIC_SUBAGENT_LIST="filemgr logwatch ping ups"
	else
		STATIC_SUBAGENT_LIST="$PROPOSED_STATIC_SUBAGENTS"
	fi

	SUBAGENT_LIBS="../subagents/filemgr/filemgr.la ../subagents/logwatch/logwatch.la ../subagents/ping/ping.la ../subagents/ups/ups.la"

	check_substr "$STATIC_SUBAGENT_LIST" "asterisk"
	if test $? = 0; then
		COMPONENTS="$COMPONENTS asterisk"
		SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/asterisk/asterisk.la"
	fi

	check_substr "$STATIC_SUBAGENT_LIST" "oracle"
	if test $? = 0; then
		COMPONENTS="$COMPONENTS oracle"
		SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/oracle/oracle.la"
	fi

	check_substr "$STATIC_SUBAGENT_LIST" "informix"
	if test $? = 0; then
		COMPONENTS="$COMPONENTS informix"
		SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/informix/informix.la"
	fi
	
	check_substr "$STATIC_SUBAGENT_LIST" "db2"
	if test $? = 0; then
		COMPONENTS="$COMPONENTS db2"
		SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/db2/db2.la"
	fi

	check_substr "$STATIC_SUBAGENT_LIST" "mysql"
	if test $? = 0; then
		COMPONENTS="$COMPONENTS mysql"
		SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/mysql/mysql.la"
	fi

	check_substr "$STATIC_SUBAGENT_LIST" "pgsql"
	if test $? = 0; then
		COMPONENTS="$COMPONENTS pgsql"
		SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/pgsql/pgsql.la"
	fi

	case "$PLATFORM" in
		Linux)
			SUBAGENT_DIRS="$SUBAGENT_DIRS linux ds18x20"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/linux/linux.la ../subagents/ds18x20/ds18x20.la"
			STATIC_SUBAGENT_LIST="linux ds18x20 $STATIC_SUBAGENT_LIST"
			;;
		FreeBSD)
			SUBAGENT_DIRS="$SUBAGENT_DIRS freebsd"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/freebsd/freebsd.la"
			STATIC_SUBAGENT_LIST="freebsd $STATIC_SUBAGENT_LIST"
			;;
		OpenBSD)
			SUBAGENT_DIRS="$SUBAGENT_DIRS openbsd"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/openbsd/openbsd.la"
			STATIC_SUBAGENT_LIST="openbsd $STATIC_SUBAGENT_LIST"
			;;
		NetBSD)
			SUBAGENT_DIRS="$SUBAGENT_DIRS netbsd"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/netbsd/netbsd.la"
			STATIC_SUBAGENT_LIST="netbsd $STATIC_SUBAGENT_LIST"
			;;
		SunOS)
			SUBAGENT_DIRS="$SUBAGENT_DIRS sunos"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/sunos/sunos.la"
			STATIC_SUBAGENT_LIST="sunos $STATIC_SUBAGENT_LIST"
			;;
		AIX)
			SUBAGENT_DIRS="$SUBAGENT_DIRS aix"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/aix/aix.la"
			STATIC_SUBAGENT_LIST="aix $STATIC_SUBAGENT_LIST"
			;;
		Darwin)
			SUBAGENT_DIRS="$SUBAGENT_DIRS darwin"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/darwin/darwin.la"
			STATIC_SUBAGENT_LIST="darwin $STATIC_SUBAGENT_LIST"
			;;
		Minix)
			SUBAGENT_DIRS="$SUBAGENT_DIRS minix"
			SUBAGENT_LIBS="$SUBAGENT_LIBS ../subagents/minix/minix.la"
			STATIC_SUBAGENT_LIST="minix $STATIC_SUBAGENT_LIST"
			;;
		*)
			# unknown
			;;
	esac
fi

check_substr "$COMPONENTS" "agent"
if test $? = 0; then
	BUILD_AGENT="yes"
	MODULES="$MODULES appagent libnxlp db agent"
	TEST_MODULES="$TEST_MODULES agent"
	TOOLS="$TOOLS nxlptest"

	case "$PLATFORM" in
		Linux)
			SUBAGENT_DIRS="$SUBAGENT_DIRS linux ds18x20"
			AGENT_UNIT_TESTS="$AGENT_UNIT_TESTS linux-cpu-usage-collector"
			;;
		FreeBSD)
			SUBAGENT_DIRS="$SUBAGENT_DIRS freebsd"
			;;
		OpenBSD)
			SUBAGENT_DIRS="$SUBAGENT_DIRS openbsd"
			;;
		NetBSD)
			SUBAGENT_DIRS="$SUBAGENT_DIRS netbsd"
			;;
		SunOS)
			SUBAGENT_DIRS="$SUBAGENT_DIRS sunos"
			;;
		AIX)
			SUBAGENT_DIRS="$SUBAGENT_DIRS aix"
			;;
		Darwin)
			SUBAGENT_DIRS="$SUBAGENT_DIRS darwin"
			;;
		Minix)
			SUBAGENT_DIRS="$SUBAGENT_DIRS minix"
			;;
		*)
			# unknown
			;;
	esac
else
	SYSTEMD_SUPPORT="no"
fi

check_substr "$COMPONENTS" "client"
if test $? = 0; then
	BUILD_CLIENT="yes"
	MODULES="$MODULES client"
	CLIENT_COMPONENTS="$CLIENT_COMPONENTS nxalarm nxevent nxpush nxnotify"
	check_substr "$COMPONENTS" "java"
	if test $? = 0; then
		CLIENT_COMPONENTS="$CLIENT_COMPONENTS java nxshell"
		if test \( "x$PLATFORM" = "xLinux" -o "x$PLATFORM" = "xDarwin" \) -a "x$BUILD_GUI_CLIENT" = "xyes"; then
			CLIENT_COMPONENTS="$CLIENT_COMPONENTS nxmc"
		fi
	fi
fi

check_substr "$COMPONENTS" "server"
if test $? = 0; then
	if test "x$DBDRIVER_SELECTED" != "xyes"; then
		AC_MSG_ERROR(You must select at least one database driver when building server.)
	fi
	if test "x$DISABLE_ENCRYPTION" = "xyes"; then
		AC_MSG_ERROR(Encryption support is required for server build.)
	fi
	if test "x$BUILD_UNICODE" = "xno"; then
		AC_MSG_ERROR(Only UNICODE server builds are supported.)
	fi

	BUILD_SERVER="yes"
	MODULES="$MODULES libnxsl server ncdrivers nxscript"
	TEST_MODULES="$TEST_MODULES test-libnxsl"
	TOP_LEVEL_MODULES="$TOP_LEVEL_MODULES sql images"
	CONTRIB_MODULES="$CONTRIB_MODULES mibs backgrounds music oui templates"
	NCDRV_MODULES="$NCDRV_MODULES nxagent"
	PDSDRV_DIRS="clickhouse influxdb"

	check_substr "$COMPONENTS" "java"
	if test $? = 0; then
		check_substr "$COMPONENTS" "client"
		if test $? = 0; then
			SERVER_COMPONENTS="nxreportd $SERVER_COMPONENTS"
		fi
	fi
fi

check_substr "$COMPONENTS" "client-proxy"
if test $? = 0; then
	MODULES="$MODULES nxcproxy"
fi

check_substr "$COMPONENTS" "mysql"
if test $? = 0; then
	if test "x$MARIADB_MYSQL_CONFLICT" = "xyes"; then
		AC_MSG_ERROR(Selected configuration options prevent building of both MariaDB and MySQL drivers)
	fi
	DB_DRIVERS="$DB_DRIVERS mysql"
	SUBAGENT_DIRS="$SUBAGENT_DIRS mysql"
fi

check_substr "$COMPONENTS" "mariadb"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS mariadb"
	SUBAGENT_DIRS="$SUBAGENT_DIRS mysql"
fi

check_substr "$COMPONENTS" "pgsql"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS pgsql"
	SUBAGENT_DIRS="$SUBAGENT_DIRS pgsql"
fi

check_substr "$COMPONENTS" "sqlite_drv"
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
	SUBAGENT_DIRS="$SUBAGENT_DIRS oracle"
fi

check_substr "$COMPONENTS" "db2"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS db2"
    SUBAGENT_DIRS="$SUBAGENT_DIRS db2"
fi

check_substr "$COMPONENTS" "informix"
if test $? = 0; then
	DB_DRIVERS="$DB_DRIVERS informix"
	SUBAGENT_DIRS="$SUBAGENT_DIRS informix"
fi

check_substr "$COMPONENTS" "raspberrypi"
if test $? = 0; then
	SUBAGENT_DIRS="$SUBAGENT_DIRS rpi"
fi

check_substr "$COMPONENTS" "tuxedo"
if test $? = 0; then
	AGENT_DIRS="$AGENT_DIRS libnxtux"
	SUBAGENT_DIRS="$SUBAGENT_DIRS tuxedo"
fi

check_substr "$COMPONENTS" "mongodb"
if test $? = 0; then
	SUBAGENT_DIRS="$SUBAGENT_DIRS mongodb"
fi

check_substr "$COMPONENTS" "redis"
if test $? = 0; then
	SUBAGENT_DIRS="$SUBAGENT_DIRS redis"
fi

check_substr "$COMPONENTS" "vmgr"
if test $? = 0; then
	SUBAGENT_DIRS="$SUBAGENT_DIRS vmgr"
fi

check_substr "$COMPONENTS" "xen"
if test $? = 0; then
	SUBAGENT_DIRS="$SUBAGENT_DIRS xen"
fi

check_substr "$COMPONENTS" "asterisk"
if test $? = 0; then
	SUBAGENT_DIRS="$SUBAGENT_DIRS asterisk"
fi

check_substr "$COMPONENTS" "sdk"
if test $? = 0; then
	if test "x$STATIC_BUILD" = "xyes"; then
		MODULES="$MODULES libnxmb appagent client"
	else
		MODULES="$MODULES libnxmb appagent client libnxsl libnxlp libnxcc db nxscript ncdrivers snmp"
		TEST_MODULES="$TEST_MODULES test-libnxcc test-libnxsl test-libnxsnmp"
		TOOLS="$TOOLS nxlptest"
	fi
	AGENT_DIRS="$AGENT_DIRS libnxappc"
fi

MODULES="$MODULES tools"


#--------------------------------------------------------------------
# Check for programs
#--------------------------------------------------------------------

if test "x$PLATFORM" = "xAIX"; then
	AC_CHECK_PROG([CC], [xlclang], [xlclang])
	AC_CHECK_PROG([CCC], [xlclang++], [xlclang++])
fi
if test "x$PLATFORM" = "xSunOS"; then
	AC_CHECK_PROG([CC], [cc], [cc])
	AC_CHECK_PROG([CCC], [CC], [CC])
fi
if test "x$PLATFORM" = "xMinix"; then
	AC_CHECK_PROG([CC], [clang], [clang])
	AC_CHECK_PROG([CCC], [clang++], [clang++])
fi
if test "x$PLATFORM" = "xLinux" -a "x$ALLOW_CLANG" = "xyes"; then
	AC_CHECK_PROG([CC], [clang-8], [clang-8])
	AC_CHECK_PROG([CC], [clang-7], [clang-7])
	AC_CHECK_PROG([CC], [clang], [clang])
	AC_CHECK_PROG([CCC], [clang++-8], [clang++-8])
	AC_CHECK_PROG([CCC], [clang++-7], [clang++-7])
	AC_CHECK_PROG([CCC], [clang++], [clang++])
fi
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
if test "x$CXX" = "xg++"; then
	AC_CHECK_PROG([GPLUSPLUS], [g++], [yes], [no])
	if test "x$GPLUSPLUS" = "xno"; then
		AC_MSG_ERROR([*** FATAL: Cannot find any usable C++ compiler])
	fi
fi
if test "x$CXX" = "xclang++"; then
	AC_LANG_PUSH([C++])

	AC_MSG_CHECKING(whether -lc++ is needed)
	AC_RUN_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
		[ AC_MSG_RESULT(no) ], 
		[ 
			OLDLIBS="$LIBS"
			LIBS="$LIBS -lc++"
			AC_RUN_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
				[ AC_MSG_RESULT(yes) ], 
				[
					LIBS="$OLDLIBS"
					AC_MSG_RESULT(no)
				])
		])

	AC_LANG_POP([C++])
fi
AM_PROG_AS
AC_PROG_INSTALL
AC_PROG_LEX(noyywrap)
AC_CHECK_PROGS(YACC,bison byacc,yacc)
AC_PROG_LN_S

AC_ARG_VAR(PERL,local path to the perl interpreter)
perl_possible_path="/usr/bin:/usr/local/bin:/bin:/opt/perl/bin:/opt/perl/usr/bin:/opt/perl/usr/local/bin"
AC_PATH_PROG(PERL,perl,/usr/bin/env perl,$perl_possible_path)


#--------------------------------------------------------------------
# Private extensions
#--------------------------------------------------------------------

if test "x$WITH_PRIVATE_EXTENSIONS" = "xyes"; then
	CXXFLAGS="$CXXFLAGS -DCUSTOM_INIT_CODE"
fi


#--------------------------------------------------------------------
# Platform-dependent settings
#--------------------------------------------------------------------

case "$PLATFORM" in
   AIX)
      CPPFLAGS="$CPPFLAGS -D_THREAD_SAFE=1"
      LDFLAGS_PREFIX="-no-undefined -Wl,-brtl $LDFLAGS"
      LIBNXAGENT_LIBS="-lodm -lcfg"
      ;;
   SunOS)
      CPPFLAGS="$CPPFLAGS -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D__EXTENSIONS__"
      AC_CPP_CHECK_OPTION(-O2)
      AC_C_CHECK_OPTION(-features=extensions)
      ;;
   Linux)
      CPPFLAGS="$CPPFLAGS -D_GNU_SOURCE -O2"
      ;;
   Darwin)
      CPPFLAGS="$CPPFLAGS -Wno-deprecated-declarations -O2 -D__STDC_WANT_LIB_EXT1__=1"
      ;;
   Minix)
      CPPFLAGS="$CPPFLAGS -I/usr/pkg/include"
      LDFLAGS="$LDFLAGS -L/usr/pkg/lib"
      USE_PTH="yes"
      ;;
   *)
      ;;
esac


#--------------------------------------------------------------------
# C++ compiler tuning:
#  * Set correct language level
#  * Disable C++ exceptions and RTTI
#  * Turn on 64bit mode on AIX
#  * Turn on 64bit mode on Solaris
#  * Disable inconsistent C/C++ linkage warnings for Solaris Studio
#--------------------------------------------------------------------

AC_C_CONST
AC_PROG_GCC_TRADITIONAL
AC_C_BIGENDIAN

AC_MSG_CHECKING(whether C compiler supports "unused" attribute)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ static int __attribute__ ((unused)) z = 0; ]], [[ ]])],
	[
		AC_DEFINE(UNUSED_ATTRIBUTE_SUPPORTED, 1, Define to 1 if compiler supports "unused" attribute)
		AC_MSG_RESULT(yes)
	],
	[
		AC_MSG_RESULT(no)
])

if test "x$ac_compiler_gnu" = "xyes" ; then
	SQLFLAGS="-x c -Wundef"
	CPPFLAGS="$CPPFLAGS -Wformat"

	if test "x$CXX" = "xg++"; then
		SQLFLAGS="-P $SQLFLAGS"
	fi

	AC_LANG_PUSH([C++])
	AC_MSG_CHECKING(whether C++ compiler accepts -std=c++17)
	OLD_CXXFLAGS="$CXXFLAGS"
	CXXFLAGS="$CXXFLAGS -std=c++17 -Werror"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
		[
			CXXFLAGS="$OLD_CXXFLAGS -std=c++17"
			AC_MSG_RESULT(yes)
		], 
		[ 
			CXXFLAGS="$OLD_CXXFLAGS"
			AC_MSG_RESULT(no)

			AC_MSG_CHECKING(whether C++ compiler accepts -std=c++14)
			OLD_CXXFLAGS="$CXXFLAGS"
			CXXFLAGS="$CXXFLAGS -std=c++14 -Werror"
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
				[
					CXXFLAGS="$OLD_CXXFLAGS -std=c++14"
					AC_MSG_RESULT(yes)
				], 
				[ 
					CXXFLAGS="$OLD_CXXFLAGS"
					AC_MSG_RESULT(no)

					AC_MSG_CHECKING(whether C++ compiler accepts -std=c++11)
					OLD_CXXFLAGS="$CXXFLAGS"
					CXXFLAGS="$CXXFLAGS -std=c++11 -Werror"
					AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
						[
							CXXFLAGS="$OLD_CXXFLAGS -std=c++11"
							AC_MSG_RESULT(yes)
						],
						[ 
							CXXFLAGS="$OLD_CXXFLAGS"
							AC_MSG_RESULT(no)
						])
				])
		])
	AC_LANG_POP([C++])

	AC_CPP_CHECK_OPTION(-Wno-unused-result)
	AC_CPP_CHECK_OPTION(-Wno-format-truncation)
	AC_CPP_CHECK_OPTION(-Wunused-but-set-variable)
	AC_CPP_CHECK_OPTION(-Wunused-variable)
	AC_CPP_CHECK_OPTION(-Wunused-value)
	AC_CPP_CHECK_OPTION(-Werror=return-type)
	AC_CXX_CHECK_OPTION(-Wreorder)
	AC_CXX_CHECK_OPTION(-Waddress)
	AC_CXX_CHECK_OPTION(-Wnon-pod-varargs)
	AC_CXX_CHECK_OPTION(-Wdeprecated-copy)
	AC_CXX_CHECK_OPTION(-fno-exceptions)
	if test "x$ENABLE_RTTI" != "xyes"; then
		AC_CXX_CHECK_OPTION(-fno-rtti)
	fi

	# Symbol visibility control on AIX with xlclang++ hides vtables for classes
	# even when visibility "default" is set for class explicitly which
	# leads to "undefined vtable for ..." linker errors
	if test "x$CXX" != "xxlclang++" -a "x$DISABLE_VISIBILITY_CONTROL" != "xyes"; then
		AC_CXX_CHECK_OPTION(-fvisibility-inlines-hidden)
		AC_CPP_CHECK_OPTION(-fvisibility=hidden)
	fi

	AC_MSG_CHECKING(whether C compiler accepts -Wno-invalid-pp-token)
	OLD_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$SQLFLAGS -Wno-invalid-pp-token"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
		[
			SQLFLAGS="$SQLFLAGS -Wno-invalid-pp-token"
			AC_MSG_RESULT(yes)
                ],
		[ AC_MSG_RESULT(no) ])
	CPPFLAGS="$OLD_CPPFLAGS"

	MACHINE=`uname -m`
	if test "x$MACHINE" = "xi686" -o "x$MACHINE" = "xi586"; then
		AC_CPP_CHECK_OPTION(-march=$MACHINE)
	fi

	if test "x$PLATFORM" = "xAIX"; then
		if test "x$CXX" = "xxlclang++"; then
			if test "x$FORCE_32BIT_BUILD" = "xyes"; then
				CPPFLAGS="$CPPFLAGS -m32"
				LDFLAGS="-m32 $LDFLAGS"
				LTINIT_CC="$CC -m32"
				LTINIT_CXX="$CXX -m32"
				OBJECT_MODE=32
			else
				CPPFLAGS="$CPPFLAGS -m64"
				LDFLAGS="-m64 $LDFLAGS"
				LTINIT_CC="$CC -m64"
				LTINIT_CXX="$CXX -m64"
				OBJECT_MODE=64
			fi
		else
			if test "x$FORCE_32BIT_BUILD" = "xyes"; then
				AC_MSG_CHECKING(whether C compiler accepts -maix32)
				OLD_CPPFLAGS="$CPPFLAGS"
				CPPFLAGS="$CPPFLAGS -maix32"
				AC_RUN_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
					[ 
						LDFLAGS="-maix32 $LDFLAGS"
						LTINIT_CC="$CC -maix32"
						LTINIT_CXX="$CXX -maix32"
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
						LTINIT_CC="$CC -maix64"
						LTINIT_CXX="$CXX -maix64"
						OBJECT_MODE=64
						AC_MSG_RESULT(yes) 
					], 
					[ 
						CPPFLAGS="$OLD_CPPFLAGS"
						AC_MSG_RESULT(no)
					])
			fi
		fi
	fi
	
	if test "x$PLATFORM" = "xSunOS"; then
		if test "x$FORCE_32BIT_BUILD" = "xyes"; then
			AC_MSG_CHECKING(whether C compiler accepts -m32)
			OLD_CPPFLAGS="$CPPFLAGS"
			CPPFLAGS="$CPPFLAGS -m32"
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
				[ 
					LDFLAGS="-m32 $LDFLAGS"
					LTINIT_CC="$CC -m32"
					LTINIT_CXX="$CXX -m32"
					AC_MSG_RESULT(yes) 
				], 
				[ 
					CPPFLAGS="$OLD_CPPFLAGS"
					AC_MSG_RESULT(no)
				])
		else
			AC_MSG_CHECKING(whether C compiler accepts -m64)
			OLD_CPPFLAGS="$CPPFLAGS"
			CPPFLAGS="$CPPFLAGS -m64"
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
				[ 
					LDFLAGS="-m64 $LDFLAGS"
					LTINIT_CC="$CC -m64"
					LTINIT_CXX="$CXX -m64"
					AC_MSG_RESULT(yes) 
				], 
				[ 
					CPPFLAGS="$OLD_CPPFLAGS"
					AC_MSG_RESULT(no)
				])
		fi
	fi

	
	if test "x$PLATFORM" = "xLinux"; then
		if test "x$FORCE_32BIT_BUILD" = "xyes"; then
			AC_MSG_CHECKING(whether C compiler accepts -m32)
			OLD_CPPFLAGS="$CPPFLAGS"
			CPPFLAGS="$CPPFLAGS -m32"
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
				[ 
					LDFLAGS="-m32 $LDFLAGS"
					LTINIT_CC="$CC -m32"
					LTINIT_CXX="$CXX -m32"
					AC_MSG_RESULT(yes) 
				], 
				[ 
					CPPFLAGS="$OLD_CPPFLAGS"
					AC_MSG_RESULT(no)
				])
		fi
	fi

	if test "x$ENABLE_DEBUG" = "xyes"; then
		AC_CPP_CHECK_OPTION(-ggdb3)
		AC_CPP_CHECK_OPTION(-fno-omit-frame-pointer)
	fi
fi

if test "x$CXX" = "xCC" -o "x$CXX" = "xCC -std=c++11" ; then
	SQLFLAGS="-Qn"
	CPPFLAGS="$CPPFLAGS -mt"
	CXXFLAGS="$CXXFLAGS -erroff=badargtype2w,wbadinit,wbadlkginit"

	TEST_XARCH="no"
	if test "x$FORCE_32BIT_BUILD" = "xyes"; then
		AC_MSG_CHECKING(whether C compiler accepts -m32)
		count=`$CC -flags | grep '^\-m32 ' | wc -l`
		if test $count -eq 1; then
			AC_MSG_RESULT(yes)
			CPPFLAGS="$CPPFLAGS -m32"
			LDFLAGS="-m32 $LDFLAGS"
		else
			AC_MSG_RESULT(no)
		fi
	else
		AC_MSG_CHECKING(whether C compiler accepts -m64)
		count=`$CC -flags | grep '^\-m64 ' | wc -l`
		if test $count -eq 1; then
			AC_MSG_RESULT(yes)
			CPPFLAGS="$CPPFLAGS -m64"
			LDFLAGS="-m64 $LDFLAGS"
		else
			AC_MSG_RESULT(no)
			TEST_XARCH="yes"
		fi
		if test "x$TEST_XARCH" = "xyes"; then
			if test "x$CPUTYPE" = "xsparc"; then
				ARCH="-xarch=v9"
			else
				ARCH="-xarch=amd64"
			fi
			AC_MSG_CHECKING(whether C compiler accepts $ARCH)
			OLD_CPPFLAGS="$CPPFLAGS"
			CPPFLAGS="$CPPFLAGS $ARCH"
			AC_RUN_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
				[ 
					LDFLAGS="$ARCH $LDFLAGS"
					AC_MSG_RESULT(yes) 
				], 
				[ 
					CPPFLAGS="$OLD_CPPFLAGS"
					AC_MSG_RESULT(no)
				])
		fi
	fi

	# Check for -xarch=sparc if -xarch was not set already
	if test "x$TEST_XARCH" = "xno" -a "x$CPUTYPE" = "xsparc"; then
		AC_MSG_CHECKING(whether C compiler accepts -xarch=sparc)
		OLD_CPPFLAGS="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS -xarch=sparc"
		AC_RUN_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
			[ 
				LDFLAGS="-xarch=sparc $LDFLAGS"
				AC_MSG_RESULT(yes) 
			], 
			[ 
				CPPFLAGS="$OLD_CPPFLAGS"
				AC_MSG_RESULT(no)
			])
	fi

	AC_CXX_CHECK_OPTION(-std=c++11)
	AC_LD_CHECK_OPTION(-std=c++11)
	AC_CXX_CHECK_OPTION(-features=no%except)
	AC_LD_CHECK_OPTION(-xatomic=studio)
	if test "x$ENABLE_RTTI" != "xyes"; then
		AC_CXX_CHECK_OPTION(-features=no%rtti)
	fi

	if test "x$DISABLE_VISIBILITY_CONTROL" != "xyes"; then
		AC_MSG_CHECKING(whether C compiler supports symbol visibility control)
		OLD_CPPFLAGS="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS -xldscope=hidden"
		AC_RUN_IFELSE([AC_LANG_PROGRAM([[ int __global z = 0; ]], [[ ]])],
			[ 
				AC_DEFINE(GLOBAL_SPECIFIER_SUPPORTED, 1, Define to 1 if compiler supports scoping specifier __global)
				AC_MSG_RESULT(yes) 
			], 
			[ 
				CPPFLAGS="$OLD_CPPFLAGS"
				AC_MSG_RESULT(no)
			])
	fi
fi

AC_LANG_PUSH([C++])

AC_MSG_CHECKING(whether C++ compiler supports std::make_unique)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <memory>
]],[[
	std::unique_ptr<int> p = std::make_unique<int>(42);
	return 0;
]])
],[
        AC_MSG_RESULT(yes)
        AC_DEFINE(HAVE_STD_MAKE_UNIQUE,1,Define to 1 if std::make_unique is available)
],[
        AC_MSG_RESULT(no)
        AC_DEFINE(HAVE_STD_MAKE_UNIQUE,0,Define to 1 if std::make_unique is available)
])

AC_LANG_POP([C++])


#--------------------------------------------------------------------
# Initialize libtool
#--------------------------------------------------------------------

if test "x$STATIC_BUILD" = "xyes"; then
	enable_shared=no
	enable_static=yes
else
	enable_shared=yes
	if test "x$BUILD_SDK" = "xyes"; then
		enable_static=yes
	else
		enable_static=no
	fi
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

if test -d /usr/local/include; then
	if test "x$ac_compiler_gnu" = "xyes"; then
		AC_MSG_CHECKING(whether -I/usr/local/include needed)
		OLD_CPPFLAGS="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS -Werror -I/usr/local/include"
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],[])],
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


#--------------------------------------------------------------------
# Check for additional system libraries
#--------------------------------------------------------------------

AC_CHECK_LIB(xnet, accept)
AC_CHECK_LIB(socket, if_nameindex)
AC_CHECK_LIB(dl, dlopen)
AC_CHECK_LIB(kstat, kstat_open)


#--------------------------------------------------------------------
# Checks for resolver functions
#--------------------------------------------------------------------

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

AC_CHECK_FUNCS([gethostbyaddr_r gethostbyname_r gethostbyname2_r getaddrinfo getnameinfo])


#--------------------------------------------------------------------
# Check for threads library - either POSIX or GNU
#--------------------------------------------------------------------

if test "x$USE_PTH" = "xyes"; then
	AC_CHECK_HEADER(pth.h,,AC_MSG_ERROR([*** GNU Pth thread support not installed - please install first ***]))
	CPPFLAGS="$CPPFLAGS -D_USE_GNU_PTH"
	LIBS="$LIBS -lpth"
else
	AC_CHECK_HEADER(pthread.h,,AC_MSG_ERROR([*** POSIX thread support not installed - please install first ***]))

	PTHREAD_LIBS="error"
	AC_MSG_CHECKING(for old style FreeBSD -pthread flag)
	AC_EGREP_CPP(yes,
		[#if defined(__FreeBSD_cc_version) || defined(__OpenBSD__)
			yes
		#endif
		], [
			AC_MSG_RESULT(yes)
			PTHREAD_LIBS="-pthread"
			if test "x$CXX" = "xc++"; then
				# Quash clang's "-pthread unused during compilation" warnings
				PTHREAD_LDFLAGS="-Wc,-Qunused-arguments"
			fi
		],
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

	if test "x$PTHREAD_LIBS" = "xerror"; then
		AC_MSG_ERROR(*** Unable to locate working posix thread library ***)
	fi
	LIBS="$LIBS $PTHREAD_LIBS"

	# Extensions to posix threads
	AC_CHECK_HEADERS([pthread_np.h],,,[
#include <pthread.h>
	])
	AC_CHECK_FUNCS([pthread_mutex_timedlock pthread_cond_reltimedwait_np])
	AC_CHECK_FUNCS([pthread_setname_np pthread_threadid_np pthread_getthreadid_np pthread_getlwpid_np])

	# Check if pthread_setname_np accepts two arguments
	if test "$ac_cv_func_pthread_setname_np" = "yes"; then
		AC_LANG_PUSH([C++])
		AC_MSG_CHECKING(if pthread_setname_np accepts two arguments)
		AC_COMPILE_IFELSE([
			AC_LANG_PROGRAM([
#include <pthread.h>
#if HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
					],
					[ pthread_setname_np(pthread_self(), "test"); ])],
			[
				AC_MSG_RESULT(yes)
				AC_DEFINE(PTHREAD_SETNAME_NP_2ARGS, 1, Define to 1 if ptread_setname_np accepts two arguments)
			],
			[
				AC_MSG_RESULT(no)
			]
		)
		AC_LANG_POP([C++])
	fi

	# Check for recursive mutexes
	AC_CHECK_FUNCS([pthread_mutexattr_settype __pthread_mutexattr_settype pthread_mutexattr_setkind_np])
	AC_CHECK_DECLS([pthread_mutexattr_settype, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_RECURSIVE_NP, PTHREAD_MUTEX_ADAPTIVE_NP, MUTEX_TYPE_COUNTING_FAST],,,[
#include <pthread.h>
#if HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
	])

	# Check for read/write locks
	if test "x$FORCE_INTERNAL_RWLOCK" != "xyes"; then
		# On Linux, rwlock functions declared always, but pthread_rwlock_t may need
		# additional defines
		AC_CHECK_FUNCS([pthread_rwlock_init pthread_rwlock_timedrdlock pthread_rwlock_timedwrlock])
		if test "$ac_cv_func_pthread_rwlock_init" = "yes"; then
			AC_CACHE_CHECK([for pthread_rwlock_t], ac_cv_struct_pthread_rw,
				[AC_COMPILE_IFELSE([
					AC_LANG_PROGRAM([
#include <sys/types.h>
#include <pthread.h>
					],
					[pthread_rwlock_t rwlock=PTHREAD_RWLOCK_INITIALIZER;])],
				ac_cv_struct_pthread_rw=yes, [
					AC_COMPILE_IFELSE([
						AC_LANG_PROGRAM([
#define _XOPEN_SOURCE 500
#include <sys/types.h>
#include <pthread.h>
						],
						[pthread_rwlock_t rwlock=PTHREAD_RWLOCK_INITIALIZER;])],
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
fi

AC_LANG_PUSH([C++])

AC_MSG_CHECKING(whether C++ compiler supports __thread)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
]],[[
	static __thread int z;
	return 0;
]])
],[
        AC_MSG_RESULT(yes)
        AC_DEFINE(HAVE_THREAD_SPECIFIER,1,Define to 1 if __thread storage specifier is available)
],[
        AC_MSG_RESULT(no)
        AC_DEFINE(HAVE_THREAD_SPECIFIER,0,Define to 1 if __thread storage specifier is available)
])

AC_MSG_CHECKING(whether C++ compiler supports thread_local)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
]],[[
	static thread_local int z;
	return 0;
]])
],[
        AC_MSG_RESULT(yes)
        AC_DEFINE(HAVE_THREAD_LOCAL_SPECIFIER,1,Define to 1 if thread_local storage specifier is available)
],[
        AC_MSG_RESULT(no)
        AC_DEFINE(HAVE_THREAD_LOCAL_SPECIFIER,0,Define to 1 if thread_local storage specifier is available)
])

AC_LANG_POP([C++])


#--------------------------------------------------------------------
# Checks for OpenSSL
#--------------------------------------------------------------------

if test "x$DISABLE_ENCRYPTION" != "xyes" -a "x$DIST_BUILD" != "xyes"; then
	AC_CHECK_HEADER(openssl/crypto.h,,AC_MSG_ERROR([OpenSSL is required for encryption support]))

	AC_CHECK_LIB($OPENSSL_LIBCRYPTO, RSA_new,
		[
			AC_DEFINE(WITH_OPENSSL, 1, Define to 1 if you have OpenSSL library and headers)
			AC_CHECK_LIB($OPENSSL_LIBCRYPTO, EVP_aes_256_cbc,
							 [HAVE_LIBCRYPTO="yes"], [AC_DEFINE(NETXMS_NO_AES,,[desc])])
			AC_CHECK_LIB($OPENSSL_LIBCRYPTO, EVP_bf_cbc,
							 [HAVE_LIBCRYPTO="yes"], [AC_DEFINE(NETXMS_NO_BF,,[desc])])
			AC_CHECK_LIB($OPENSSL_LIBCRYPTO, EVP_idea_cbc,
							 [HAVE_LIBCRYPTO="yes"], [AC_DEFINE(NETXMS_NO_IDEA,,[desc])])
			AC_CHECK_LIB($OPENSSL_LIBCRYPTO, EVP_des_ede3_cbc,
							 [HAVE_LIBCRYPTO="yes"], [AC_DEFINE(NETXMS_NO_DES,,[desc])])
			if test "x$HAVE_LIBCRYPTO" = "xyes"; then
				WITH_ENCRYPTION="yes"
				LIBS="$LIBS -l$OPENSSL_LIBSSL -l$OPENSSL_LIBCRYPTO"
			else
				AC_MSG_ERROR(lib$OPENSSL_LIBCRYPTO does not provide any suitable encryption algorithm)
			fi

			AC_CHECK_FUNCS([X509_STORE_set_verify_cb X509_STORE_CTX_set_verify_cb])
		],
		[
			AC_MSG_ERROR(lib$OPENSSL_LIBCRYPTO is required for encryption support)
		])
fi


#--------------------------------------------------------------------
# Checks for header files
#--------------------------------------------------------------------

AC_CHECK_HEADERS([sys/types.h sys/stat.h unistd.h stdarg.h fcntl.h sched.h sys/ptrace.h])
AC_CHECK_HEADERS([sys/int_types.h time.h sys/time.h sys/utsname.h sys/wait.h])
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h net/nh.h sys/socket.h])
AC_CHECK_HEADERS([fcntl.h dirent.h sys/ioctl.h sys/sockio.h poll.h termios.h])
AC_CHECK_HEADERS([inttypes.h memory.h stdint.h stdlib.h strings.h string.h ctype.h])
AC_CHECK_HEADERS([byteswap.h sys/select.h dlfcn.h locale.h])
AC_CHECK_HEADERS([sys/sysctl.h sys/param.h sys/user.h vm/vm_param.h syslog.h])
AC_CHECK_HEADERS([grp.h pwd.h malloc.h stdbool.h utime.h endian.h sys/syscall.h])
AC_CHECK_HEADERS([net/if.h net/if_arp.h net/if_dl.h net/if_types.h],,,[[
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
]])
AC_CHECK_HEADERS([netinet/tcp.h],,,[[
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
]])


#--------------------------------------------------------------------
# Checks for __get_cpuid
#--------------------------------------------------------------------

AC_CHECK_HEADERS([cpuid.h])
AC_MSG_CHECKING(for __get_cpuid)
AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if HAVE_CPUID_H
#include <cpuid.h>
#endif
]],[[
	unsigned int eax, ebx, ecx, edx;
	return __get_cpuid(0x01, &eax, &ebx, &ecx, &edx);
]])
],[
	AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_GET_CPUID,1,Define to 1 if __get_cpuid function is available)
],[
	AC_MSG_RESULT(no)
	AC_DEFINE(HAVE_GET_CPUID,0,Define to 1 if __get_cpuid function is available)
])
AC_LANG_POP([C++])


#--------------------------------------------------------------------
# Checks for alloca
#--------------------------------------------------------------------

AC_CHECK_HEADERS([alloca.h])
AC_MSG_CHECKING(for alloca)
AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_MALLOC_H
#include <malloc.h>
#endif
#if HAVE_ALLOCA_H
#include <alloca.h>
#endif
]],[[
	char *p = (char *)alloca(256);
	return (p != NULL) ? 0 : 1;
]])
],[
	AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_ALLOCA,1,Define to 1 if alloca function is available)
],[
	AC_MSG_RESULT(no)
	AC_DEFINE(HAVE_ALLOCA,0,Define to 1 if alloca function is available)
])
AC_LANG_POP([C++])


#--------------------------------------------------------------------
# Checks for d_type in struct dirent
#--------------------------------------------------------------------

AC_MSG_CHECKING(for d_type in struct dirent)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_DIRENT_H
#include <dirent.h>
#endif
]],[[
	struct dirent d;
	return d.d_type;
]])
],[
	AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_DIRENT_D_TYPE,1,Define to 1 if struct dirent has member d_type)
],[
	AC_MSG_RESULT(no)
	AC_DEFINE(HAVE_DIRENT_D_TYPE,0,Define to 1 if struct dirent has member d_type)
])
AC_CHECK_DECLS([DT_DIR, DT_REG, DT_UNKNOWN],,,[
#if HAVE_DIRENT_H
#include <dirent.h>
#endif
])


#--------------------------------------------------------------------
# Checks for IPv6 support
#--------------------------------------------------------------------

if test "x$DISABLE_IPV6" != "xyes"; then
	AC_CHECK_DECL(AF_INET6,,,[
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
	])
	if test "x$ac_cv_have_decl_AF_INET6" = "xyes"; then
		AC_CHECK_TYPE(struct sockaddr_in6, [ 
			AC_DEFINE(WITH_IPV6, 1, Define to have IPv6 support) 
			IPV6_SUPPORT="yes"
		],,[
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
		])
	fi
fi


#--------------------------------------------------------------------
# Checks for RADIUS client library
#--------------------------------------------------------------------

if test "x$BUILD_SERVER" = "xyes" ; then
	AC_CHECK_HEADERS([radcli/radcli.h])
	if test "x$ac_cv_header_radcli_radcli_h" = "xyes"; then
		AC_CHECK_LIB(radcli, rc_auth, [
			SERVER_LIBS="$SERVER_LIBS -lradcli"
			AC_DEFINE(USE_RADCLI, 1, Define to 1 if radcli is used as RADIUS client)
		])
	fi
fi


#--------------------------------------------------------------------
# XMPP
#--------------------------------------------------------------------

if test "x$DISABLE_XMPP" != "xyes"; then
	AC_CHECK_HEADER(strophe.h,HAVE_LIBSTROPHE=yes,HAVE_LIBSTROPHE=no)
	if test "x$HAVE_LIBSTROPHE" = "xyes"; then
		AC_CHECK_LIB(strophe, xmpp_ctx_new, [
			NCDRV_MODULES="$NCDRV_MODULES xmpp"
			XMPP_SUPPORT="yes"
		], [ HAVE_LIBSTROPHE=no ])
	fi
	if test "x$HAVE_LIBSTROPHE" = "xyes"; then
		OLD_LIBS="$LIBS"
		LIBS="$LIBS -lstrophe"
		AC_CHECK_FUNCS([xmpp_stanza_add_child_ex])
		LIBS="$OLD_LIBS"
	fi
fi


#--------------------------------------------------------------------
# Checks for other libs.
#--------------------------------------------------------------------

if test "x$BUILD_SERVER" = "xyes" ; then
	AC_CHECK_HEADER(histedit.h,HAVE_LIBEDIT=yes,HAVE_LIBEDIT=no)
	if test "x$HAVE_LIBEDIT" = "xyes"; then
		AC_CHECK_LIB(edit, history_winit, [
			SERVER_LIBS="$SERVER_LIBS -ledit"
			NXADM_LIBS="$NXADM_LIBS -ledit"
	   		AC_DEFINE(HAVE_LIBEDIT, 1, Define to 1 if libedit is available)
		], [ HAVE_LIBEDIT=no ])
	fi
fi

check_substr "$COMPONENTS" "sqlite"
if test $? = 0; then
	if test "x$FORCE_INTERNAL_SQLITE" = "xyes"; then
		HAVE_SQLITE=no
	else
		OLD_LIBS="$LIBS"
		HAVE_SQLITE=yes
		AC_CHECK_HEADER(sqlite3.h,,HAVE_SQLITE=no)
		if test "x$HAVE_SQLITE" = "xyes"; then
			AC_CHECK_LIB(sqlite3, sqlite3_initialize, [], [ HAVE_SQLITE=no ])
		fi
		if test "x$HAVE_SQLITE" = "xyes"; then
			if test "x$cross_compiling" = "xyes"; then
				AC_MSG_WARN([Cross-compiling mode is active, assuming libsqlite3 is thread-safe])
			else
				AC_MSG_CHECKING(if libsqlite3 is thread-safe)
				AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <sqlite3.h>
					]],[[
						return sqlite3_threadsafe() ? 0 : 1;
					]])
				],[
					AC_MSG_RESULT(yes)
				],[
					AC_MSG_RESULT(no)
					HAVE_SQLITE=no
				])
			fi
		fi
		LIBS="$OLD_LIBS"
	fi
	if test "x$HAVE_SQLITE" = "xno"; then
		MODULES="sqlite $MODULES"
	fi
fi

if test "x$DIST_BUILD" != "xyes"; then
	HAVE_ZLIB=yes
	AC_CHECK_HEADER(zlib.h,,HAVE_ZLIB=no)
	if test "x$HAVE_ZLIB" = "xyes"; then
		AC_CHECK_LIB(z, deflate, [], [ HAVE_ZLIB=no ])
	fi
	if test "x$HAVE_ZLIB" = "xyes"; then
		AC_MSG_CHECKING(whether z_stream.next_in is a pointer to const)
		AC_LANG_PUSH([C++])
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#define ZLIB_CONST
#include <zlib.h>
static const unsigned char *data = (const unsigned char *)"1 2 3";
			]],[[
				z_stream s;
				s.next_in = data;
				return 0;
			]])
			],[
			        AC_MSG_RESULT(yes)
			        AC_DEFINE(ZLIB_CONST_INPUT,1,Define to 1 if zlib accepts const pointer as z_stream input)
			],[
			        AC_MSG_RESULT(no)
			        AC_DEFINE(ZLIB_CONST_INPUT,0,Define to 1 if zlib accepts const pointer as z_stream input)
			])
		AC_LANG_POP([C++])
	fi
	if test "x$HAVE_ZLIB" = "xno"; then
		AC_MSG_ERROR(zlib is required. Check that it is installed or use --with-zlib to specify non-standard location)
	fi

	HAVE_LIBEXPAT=yes
	AC_CHECK_HEADER(expat.h,,HAVE_LIBEXPAT=no)
	AC_CHECK_LIB(expat, XML_Parse, [], [ HAVE_LIBEXPAT=no ])
	if test "x$HAVE_LIBEXPAT" = "xno"; then
		AC_MSG_ERROR(libexpat is required. Check that it is installed or use --with-libexpat to specify non-standard location)
	fi
fi

if test "x$FORCE_INTERNAL_JANSSON" = "xyes"; then
	HAVE_JANSSON=no
else
	HAVE_JANSSON=yes
	AC_CHECK_HEADER(jansson.h,,HAVE_JANSSON=no)
	if test "x$HAVE_JANSSON" = "xyes"; then
		AC_CHECK_LIB(jansson, json_object, [ HAVE_JANSSON=yes ], [ HAVE_JANSSON=no ])
	fi
fi
if test "x$HAVE_JANSSON" = "xno"; then
	MODULES="jansson $MODULES"
	AC_DEFINE(BUNDLED_LIBJANSSON,1,Define to 1 if bundled libjansson is used)
else
   AC_CHECK_DECLS([json_object_update_new],,,[
#include <jansson.h>   
   ])
fi

if test "x$DISABLE_LMSENSORS" = "xno" -a "x$DIST_BUILD" != "xyes"; then
	HAVE_LMSENSORS=yes
	AC_CHECK_HEADER(sensors/sensors.h,,HAVE_LMSENSORS=no)
	if test "x$HAVE_LMSENSORS" = "xyes"; then
		AC_CHECK_LIB(sensors, sensors_init, [ DUMMY="" ], [ HAVE_LMSENSORS=no ])
	fi
	if test "x$HAVE_LMSENSORS" = "xyes"; then
		SUBAGENT_DIRS="$SUBAGENT_DIRS lmsensors"
   else
      if test "x$REQUIRE_LMSENSORS" = "xyes"; then
   		AC_MSG_ERROR(libsensors is required for lmsensors support)
      fi
	fi
fi

if test "x$DIST_BUILD" != "xyes"; then
	AC_CHECK_HEADER(curl/curl.h,HAVE_LIBCURL=yes,HAVE_LIBCURL=no)
	if test "x$HAVE_LIBCURL" = "xyes"; then
		AC_CHECK_LIB(curl, curl_global_init, [
			HAVE_LIBCURL=yes
			LIBS="$LIBS -lcurl"
		], [ HAVE_LIBCURL=no ])
		AC_CHECK_DECLS([CURLOPT_NOSIGNAL, CURLOPT_RESOLVE, CURLOPT_XOAUTH2_BEARER, CURLOPT_PROTOCOLS_STR, CURLPROXY_HTTPS, CURLE_NOT_BUILT_IN, CURLE_UNKNOWN_OPTION, curl_version_info, curl_url],,,[
#include <curl/curl.h>
		])
	fi

	if test "x$HAVE_LIBCURL" = "xyes"; then
		NCDRV_MODULES="$NCDRV_MODULES anysms googlechat kannel mattermost msteams mymobile nexmo slack smtp smseagle telegram text2reach twilio websms"
		HDLINK_DIRS="$HDLINK_DIRS redmine"
	else
		AC_MSG_ERROR(libcurl is required)
	fi
fi

if test "x$DISABLE_SSH" != "xyes" -a "x$DIST_BUILD" != "xyes"; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	CPPFLAGS="$LIBSSH_CPPFLAGS $CPPFLAGS"
	LDFLAGS="$LIBSSH_LDFLAGS $LDFLAGS"
	AC_CHECK_HEADER(libssh/libssh.h,HAVE_LIBSSH=yes,HAVE_LIBSSH=no)
	if test "x$HAVE_LIBSSH" = "xyes"; then
		AC_CHECK_LIB(ssh, ssh_version, [ HAVE_LIBSSH=yes ], [ HAVE_LIBSSH=no ])
	fi

	if test "x$HAVE_LIBSSH" = "xyes"; then
		SUBAGENT_DIRS="$SUBAGENT_DIRS ssh"
		AC_DEFINE(HAVE_LIBSSH, 1, Define to 1 if libssh is available)
	else
		if test "x$REQUIRE_SSH" = "xyes"; then
			AC_MSG_ERROR(libssh is required for SSH support)
		fi
	fi
	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
else
	HAVE_LIBSSH=no
fi

if test "x$DISABLE_JQ" != "xyes" -a "x$DIST_BUILD" != "xyes"; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"
	CPPFLAGS="$LIBJQ_CPPFLAGS $CPPFLAGS"
	LDFLAGS="$LIBJQ_LDFLAGS $LDFLAGS"
	AC_CHECK_HEADER(jq.h, HAVE_LIBJQ=yes, HAVE_LIBJQ=no)
	if test "x$HAVE_LIBJQ" = "xyes"; then
		AC_CHECK_LIB(jq, jq_init, [ HAVE_LIBJQ=yes ], [ HAVE_LIBJQ=no ])
	fi

	if test "x$HAVE_LIBJQ" = "xyes"; then
		AC_DEFINE(HAVE_LIBJQ, 1, Define to 1 if libjq is available)
	else
      if test "x$REQUIRE_JQ" = "xyes"; then
   		AC_MSG_ERROR(libjq is required for JQ support)
      fi
	fi
	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
else
	HAVE_LIBJQ=no
fi


#--------------------------------------------------------------------
# Checks for LDAP libraries
#--------------------------------------------------------------------

if test "x$DISABLE_LDAP" != "xyes" -a "x$DIST_BUILD" != "xyes"; then
	if test "$PLATFORM" = "SunOS" -a -d /usr/include/openldap; then
		# Check for OpenLDAP in Solaris 11
		OLD_CPPFLAGS="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS -I/usr/include/openldap"
		AC_CHECK_HEADER(openldap/ldap.h,HAVE_SUNOS_OPENLDAP=yes,HAVE_SUNOS_OPENLDAP=no)
		if test "x$HAVE_SUNOS_OPENLDAP" = "xyes"; then
			AC_CHECK_LIB(ldap_r-2.4, ldap_init, [ HAVE_SUNOS_OPENLDAP=yes ], [ HAVE_SUNOS_OPENLDAP=no ])
			if test "x$HAVE_SUNOS_OPENLDAP" = "xyes"; then
				AC_CHECK_LIB(lber-2.4, ber_init, [ SERVER_LIBS="-llber-2.4 $SERVER_LIBS" ])
			fi
		fi
		if test "x$HAVE_SUNOS_OPENLDAP" = "xyes"; then
			HAVE_LDAP="yes"
			SERVER_LIBS="-lldap_r-2.4 $SERVER_LIBS"   
			AC_DEFINE(WITH_LDAP, 1, Define to 1 if you have LDAP libraries and headers)
			AC_DEFINE(SUNOS_OPENLDAP, 1, Define to 1 if you are using OpenLDAP libraries and headers on SunOS)
			AC_CHECK_TYPES([ber_int_t],,,
				[
#include <openldap/ldap.h>
				])

			OLD_LIBS="$LIBS"
			LIBS="$LIBS -lldap_r-2.4"
			AC_CHECK_FUNCS([ldap_initialize ldapssl_init])
			AC_CHECK_FUNCS([ldap_control_create ldap_create_control nsldapi_build_control])
			AC_CHECK_FUNCS([ldap_create_page_control ldap_parse_page_control])
			LIBS="$OLD_LIBS"

			AC_CHECK_DECLS([nsldapi_build_control],,,
				[
#include <openldap/ldap.h>
				])
		else
			CPPFLAGS="$OLD_CPPFLAGS"
		fi
	fi

	if test "x$HAVE_SUNOS_OPENLDAP" != "xyes"; then
		AC_CHECK_HEADER(ldap.h,HAVE_LDAP=yes,HAVE_LDAP=no)
		if test "x$HAVE_LDAP" = "xyes"; then
			AC_CHECK_LIB(ldap, ldap_init, [ HAVE_LDAP=yes ], [ HAVE_LDAP=no ])
			if test "x$HAVE_LDAP" = "xyes"; then
				AC_CHECK_LIB(lber, ber_init, [ SERVER_LIBS="-llber $SERVER_LIBS" ])
			fi
		fi
		if test "x$HAVE_LDAP" = "xyes"; then
			SERVER_LIBS="-lldap $SERVER_LIBS"   
			AC_DEFINE(WITH_LDAP, 1, Define to 1 if you have LDAP libraries and headers)
			AC_CHECK_HEADERS([ldap_ssl.h],,,
				[
#include <ldap.h>
				])
			AC_CHECK_TYPES([ber_int_t],,,
				[
#include <ldap.h>
#if HAVE_LDAP_SSL_H
#include <ldap_ssl.h>
#endif
				])

			OLD_LIBS="$LIBS"
			LIBS="$LIBS -lldap"
			AC_CHECK_FUNCS([ldap_initialize ldapssl_init])
			AC_CHECK_FUNCS([ldap_control_create ldap_create_control nsldapi_build_control])
			AC_CHECK_FUNCS([ldap_create_page_control ldap_parse_page_control])
			LIBS="$OLD_LIBS"

			AC_CHECK_DECLS([nsldapi_build_control],,,
				[
#include <ldap.h>
#if HAVE_LDAP_SSL_H
#include <ldap_ssl.h>
#endif
				])
		fi
	fi

	if test "x$HAVE_LDAP" != "xyes" -a "x$REQUIRE_LDAP" = "xyes"; then
		AC_MSG_ERROR(OpenLDAP is required for LDAP support)
	fi
	LDAP_SUPPORT="$HAVE_LDAP"
fi


#--------------------------------------------------------------------
# Checks for MODBUS libraries
#--------------------------------------------------------------------

if test "x$DISABLE_MODBUS" != "xyes" -a "x$DIST_BUILD" != "xyes"; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"
	CPPFLAGS="$LIBMODBUS_CPPFLAGS $CPPFLAGS"
	LDFLAGS="$LIBMODBUS_LDFLAGS $LDFLAGS"

	AC_CHECK_HEADER(modbus/modbus.h,
		[
			HAVE_MODBUS=yes
			AC_DEFINE(HAVE_MODBUS_MODBUS_H, 1, Define to 1 if modbus/modbus.h is available)
		], HAVE_MODBUS=no)
	if test "x$HAVE_MODBUS" = "xno"; then
		AC_CHECK_HEADER(modbus.h,
			[
				HAVE_MODBUS=yes
				AC_DEFINE(HAVE_MODBUS_H, 1, Define to 1 if modbus.h is available)
			], HAVE_MODBUS=no)
	fi
	if test "x$HAVE_MODBUS" = "xyes"; then
		AC_CHECK_LIB(nxmodbus, modbus_connect, [ HAVE_MODBUS=yes ], [ HAVE_MODBUS=no ])
	fi

	if test "x$HAVE_MODBUS" = "xyes"; then
		LIBS="$LIBS -lnxmodbus"
                AC_CHECK_FUNC(modbus_read_device_id, , [
                        AC_MSG_ERROR([*** Cannot find modbus_read_device_id function - check your libnxmodbus installation or disable MODBUS support with --disable-modbus ***])
                ])
		AC_DEFINE(WITH_MODBUS, 1, Define to 1 if MODBUS support is enabled)
		SERVER_LIBS="$SERVER_LIBS -lnxmodbus"
		LIBNXSRV_LIBS="$LIBNXSRV_LIBS -lnxmodbus"
		AGENT_LIBS="$AGENT_LIBS -lnxmodbus"
		LIBNXAGENT_LIBS="$LIBNXAGENT_LIBS -lnxmodbus"
	else
		if test "x$REQUIRE_MODBUS" = "xyes"; then
			AC_MSG_ERROR([libnxmodbus (NetXMS fork of libmodbus) is required for MODBUS support])
		fi
	fi

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
else
	HAVE_MODBUS=no
fi


#--------------------------------------------------------------------
# Checks for libisotree
#--------------------------------------------------------------------

if test "x$DISABLE_ISOTREE" != "xyes" -a "x$DIST_BUILD" != "xyes" -a "x$BUILD_SERVER" = "xyes"; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"
	CPPFLAGS="$LIBISOTREE_CPPFLAGS $CPPFLAGS"
	LDFLAGS="$LIBISOTREE_LDFLAGS $LDFLAGS"

   AC_LANG_PUSH([C++])

   AC_CHECK_HEADER(isotree_oop.hpp,
		[
			HAVE_LIBISOTREE=yes
			AC_DEFINE(HAVE_ISOTREE_OOP_HPP, 1, Define to 1 if isotree_oop.hpp is available)
		], HAVE_LIBISOTREE=no)
	if test "x$HAVE_LIBISOTREE" = "xyes"; then
		AC_CHECK_LIB(isotree, isotree_predict, [ HAVE_LIBISOTREE=yes ], [ HAVE_LIBISOTREE=no ])
	fi

	if test "x$HAVE_LIBISOTREE" = "xyes"; then
		AC_DEFINE(WITH_LIBISOTREE, 1, Define to 1 if libisotree is available)
		SERVER_LIBS="$SERVER_LIBS -lisotree"
	else
		if test "x$REQUIRE_ISOTREE" = "xyes"; then
			AC_MSG_ERROR([libisotree is required])
		fi
	fi

   AC_LANG_POP([C++])

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
else
	HAVE_LIBISOTREE=no
fi


#--------------------------------------------------------------------
# Checks for systemd libraries
#--------------------------------------------------------------------

if test "x$SYSTEMD_SUPPORT" = "xyes"; then
	AC_CHECK_HEADER(systemd/sd-bus.h, [ HAVE_SDBUS=yes ], [ HAVE_SDBUS=no ])
	if test "x$HAVE_SDBUS" = "xyes"; then
		AC_CHECK_LIB(systemd, sd_bus_open_system, [ HAVE_SDBUS=yes ], [ HAVE_SDBUS=no ])
	fi
	if test "x$HAVE_SDBUS" = "xyes"; then
		AC_DEFINE(HAVE_SDBUS, 1, Define to 1 if you have Systemd SD-Bus libraries and headers)
		AGENT_LIBS="$AGENT_LIBS -lsystemd"
		SYSTEMD_LIBS="-lsystemd"
	fi
	AC_DEFINE(WITH_SYSTEMD, 1, Define to 1 to enable systemd support)
fi


#--------------------------------------------------------------------
# Checks for data types
#--------------------------------------------------------------------

AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(void*)

AC_STRUCT_TIMEZONE

AC_CHECK_TYPES([long long, unsigned long long, int64_t, uint64_t, int32_t, uint32_t, int16_t, uint16_t, int8_t, uint8_t, intptr_t, uintptr_t, bool],,,[
#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
])
AC_CHECK_TYPES([mode_t, size_t, ssize_t, off_t, off64_t, socklen_t, uuid_t],,,[
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

AC_MSG_CHECKING(if ssize_t differs from int32_t in overloads)
AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

static inline int func(int32_t v)
{
	return v + 1;
}

static inline int func(ssize_t v)
{
	return (int)v + 2;
}
]],[[
	ssize_t v = 1;
	return func(v);
]])
],[
        AC_MSG_RESULT(yes)
        AC_DEFINE(CAN_OVERLOAD_SSIZE_T,1,Define to 1 if ssize_t can be used in overloads)
],[
        AC_MSG_RESULT(no)
        AC_DEFINE(CAN_OVERLOAD_SSIZE_T,0,Define to 1 if ssize_t can be used in overloads)
])
AC_LANG_POP([C++])

AC_MSG_CHECKING(if int8_t differs from char in overloads)
AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

static inline int func(char v)
{
	return (int)v + 1;
}

static inline int func(int8_t v)
{
	return (int)v + 2;
}
]],[[
	int8_t v = 1;
	return func(v);
]])
],[
        AC_MSG_RESULT(yes)
        AC_DEFINE(CAN_OVERLOAD_INT8_T,1,Define to 1 if int8_t can be used in overloads)
],[
        AC_MSG_RESULT(no)
        AC_DEFINE(CAN_OVERLOAD_INT8_T,0,Define to 1 if int8_t can be used in overloads)
])
AC_LANG_POP([C++])

AC_MSG_CHECKING(whether va_list is a pointer)
AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
int __test(va_list v)
{
	return (v != NULL) ? 1 : 0;
}
]],[[
	return __test(NULL);
]])
],[
        AC_MSG_RESULT(yes)
        AC_DEFINE(VA_LIST_IS_POINTER,1,Define to 1 if va_list type is a pointer)
],[
        AC_MSG_RESULT(no)
        AC_DEFINE(VA_LIST_IS_POINTER,0,Define to 1 if va_list type is a pointer)
])
AC_LANG_POP([C++])


#--------------------------------------------------------------------
# Checks for functions
#--------------------------------------------------------------------

AC_FUNC_ERROR_AT_LINE
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF

AC_CHECK_FUNCS([gettimeofday memmem strcspn strrchr strlwr strtok_r strtoll strtoull])
AC_CHECK_FUNCS([strlcpy strlcat strcasestr setlocale strerror_r toupper])
AC_CHECK_FUNCS([tolower if_nametoindex daemon mmap scandir uname poll])
AC_CHECK_FUNCS([usleep nanosleep gmtime_r localtime_r stat64 fstat64 lstat64])
AC_CHECK_FUNCS([strptime timegm rand_r])
AC_CHECK_FUNCS([isatty malloc_info malloc_trim utime tzset])
AC_CHECK_FUNCS([getpwnam getpwuid getpwuid_r getgrnam getgrgid getgrgid_r])
AC_CHECK_FUNCS([getpeereid sched_yield getpid localeconv])
AC_CHECK_FUNCS([setenv unsetenv])

AC_CHECK_DECLS([nanosleep, daemon, toupper, tolower, explicit_bzero, memset_s],,,[
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
])


# Do not use inet_pton on Solaris as it has binary compatibility problems
if test "$PLATFORM" != "SunOS"; then
	AC_CHECK_FUNCS([inet_pton])
fi

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

AC_CHECK_FUNCS([sysctlbyname sysctlnametomib])
AC_CHECK_FUNCS([tcgetattr tcsetattr cfsetospeed cfsetispeed])

# asprintf, scprintf, etc.
AC_CHECK_FUNCS([asprintf aswprintf vasprintf vaswprintf])
AC_CHECK_FUNCS([scprintf scwprintf vscprintf vscwprintf])
AC_CHECK_FUNCS([snprintf vsnprintf])

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

if test "x$FORCE_NULL_SAFE_FREE" = "xyes"; then
	AC_DEFINE(FREE_IS_NULL_SAFE, 1, Define to 1 if free corectly handles NULL pointer)
else
	AC_MSG_CHECKING(whether free correctly handles NULL pointer)
	AC_RUN_IFELSE(
		[AC_LANG_PROGRAM([ 
#include <stdlib.h>
			], [
				free(NULL);
				return 0;
			])
		],
		[
			AC_MSG_RESULT(yes)
			AC_DEFINE(FREE_IS_NULL_SAFE, 1, Define to 1 if free corectly handles NULL pointer)
		],
		[ AC_MSG_RESULT(no) ],
		[ AC_MSG_RESULT(no) ]
	)
fi


#--------------------------------------------------------------------
# Checks for macros and definitions
#--------------------------------------------------------------------

AC_CHECK_DECLS([bswap_16, bswap_32, bswap_64, htonll, ntohll],,,[
#if HAVE_SYS_TYPES_H
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
AC_CHECK_DECLS([IP_MTU_DISCOVER, IP_DONTFRAG],,,[
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])
AC_CHECK_DECLS([RTLD_NOW, RTLD_GLOBAL],,,[
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif
])
AC_CHECK_DECLS([va_copy, __va_copy],,,[
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
])


#--------------------------------------------------------------------
# GCC __sync and __atomic builtins
# Exclude Oracle Developer Studio C++ because it can have those
# built-ins but they are incompatible with -xatomic=studio and
# breaks build of bundled libjansson
#--------------------------------------------------------------------

if test "$CXX" != "CC"; then
	AC_MSG_CHECKING([for gcc __sync builtins])
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],
		[[unsigned long val; __sync_bool_compare_and_swap(&val, 0, 1);]])],
		[have_sync_builtins=yes],
		[have_sync_builtins=no])
	if test "x$have_sync_builtins" = "xyes"; then
		AC_DEFINE([HAVE_SYNC_BUILTINS], [1], [Define to 1 if gcc __sync builtins are available])
	fi
	AC_MSG_RESULT([$have_sync_builtins])

	AC_MSG_CHECKING([for gcc __atomic builtins])
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],
		[[
			unsigned long v;
			__atomic_exchange_n(&v, 1, __ATOMIC_SEQ_CST);
			__atomic_store_n(&v, 1, __ATOMIC_RELEASE);
			__atomic_load_n(&v, __ATOMIC_ACQUIRE);
			__atomic_fetch_add(&v, 1, __ATOMIC_SEQ_CST);
		]])],
		[have_atomic_builtins=yes],
		[have_atomic_builtins=no])
	if test "x$have_atomic_builtins" = "xyes"; then
		AC_DEFINE([HAVE_ATOMIC_BUILTINS], [1], [Define to 1 if gcc __atomic builtins are available])
	fi
	AC_MSG_RESULT([$have_atomic_builtins])

	if test "x$ac_compiler_gnu" = "xyes" -a "$have_sync_builtins" = "yes"; then
		AC_MSG_CHECKING([if 64 bit __atomic builtins require libatomic])
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],
			[[
#if SIZEOF_LONG == 8
				unsigned long val;
#else
				unsigned long long val;
#endif
				__atomic_fetch_add(&val, 1, __ATOMIC_SEQ_CST);
			]])],
			[ AC_MSG_RESULT(no) ],
			[
				AC_MSG_RESULT(yes)
				AC_MSG_CHECKING([for libatomic])
				OLD_LIBS="$LIBS"
				LIBS="$LIBS -latomic"
				AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],
					[[
#if SIZEOF_LONG == 8
						unsigned long val;
#else
						unsigned long long val;
#endif
						__atomic_fetch_add(&val, 1, __ATOMIC_SEQ_CST);
					]])],
					[ AC_MSG_RESULT(yes) ],
					[
						AC_MSG_RESULT(no)
						LIBS="$OLD_LIBS"
					])
			])
	fi

	if test "x$ac_compiler_gnu" = "xyes" -a "$have_sync_builtins" = "yes" -a "$have_atomic_builtins" != "yes"; then
		AC_MSG_CHECKING([if 64 bit __sync builtins require libatomic])
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],
			[[
#if SIZEOF_LONG == 8
				unsigned long val;
#else
				unsigned long long val;
#endif
				__sync_fetch_and_add(&val, 1);
			]])],
			[ AC_MSG_RESULT(no) ],
			[
				AC_MSG_RESULT(yes)
				AC_MSG_CHECKING([for libatomic])
				OLD_LIBS="$LIBS"
				LIBS="$LIBS -latomic"
				AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],
					[[
#if SIZEOF_LONG == 8
						unsigned long val;
#else
						unsigned long long val;
#endif
						__sync_fetch_and_add(&val, 1);
					]])],
					[ AC_MSG_RESULT(yes) ],
					[
						AC_MSG_RESULT(no)
						LIBS="$OLD_LIBS"
					])
			])
	fi
fi


#--------------------------------------------------------------------
# getopt
#--------------------------------------------------------------------

if test "x$FORCE_INTERNAL_GETOPT" = "xyes"; then
	AC_DEFINE([USE_BUNDLED_GETOPT], [1], [Define to 1 if bundled getopt must be used])
else
	AC_CHECK_HEADERS([getopt.h])
	AC_CHECK_LIB(gnugetopt, getopt_long)
	AC_CHECK_DECLS([getopt_long],,,[
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
	])
fi


#--------------------------------------------------------------------
# getsockopt
#--------------------------------------------------------------------

AC_MSG_CHECKING([if getsockopt uses socklen_t])
AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE([
	AC_LANG_PROGRAM([
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
	],
	[
		unsigned int err;
		socklen_t len = sizeof(int);
		return getsockopt(0, SOL_SOCKET, SO_ERROR, &err, &len);
	])],
	[
		AC_MSG_RESULT(yes)
		AC_DEFINE(GETSOCKOPT_USES_SOCKLEN_T, 1, Define to 1 if getsockopt uses socklen_t for length argument)
	],
	[
		AC_MSG_RESULT(no)
		AC_DEFINE(GETSOCKOPT_USES_SOCKLEN_T, 0, Define to 1 if getsockopt uses socklen_t for length argument)
	]
)
AC_LANG_POP([C++])


#--------------------------------------------------------------------
# Linux specific checks
#--------------------------------------------------------------------

if test "x$PLATFORM" = "xLinux"; then
	AC_CHECK_HEADERS([sys/reboot.h],,,[[ ]])
	AC_CHECK_DECLS([reboot, RB_AUTOBOOT, RB_POWER_OFF, RB_HALT_SYSTEM],,,[
#if HAVE_SYS_REBOOT_H
#include <sys/reboot.h>
#endif
	])
	AC_CHECK_FUNCS([reboot],,,[
#if HAVE_SYS_REBOOT_H
#include <sys/reboot.h>
#endif
	])

	AC_MSG_CHECKING(for gettid syscall)
	AC_LANG_PUSH([C++])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ 
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif
			]], [[ 
syscall(SYS_gettid);
			]])],
		[ 
			AC_DEFINE([HAVE_GETTID_SYSCALL],[1],[Define to 1 if gettid syscall is available])
			AC_MSG_RESULT(yes) 
		], 
		[ 
			AC_MSG_RESULT(no)
		])
	AC_LANG_POP([C++])
fi


#--------------------------------------------------------------------
# AIX specific checks
#--------------------------------------------------------------------

if test "x$PLATFORM" = "xAIX"; then
	AC_CHECK_HEADERS([procinfo.h],,,[[ ]])
	AC_CHECK_DECLS([getkerninfo, getprocs, getprocs64],,,[
#if HAVE_PROCINFO_H
#include <procinfo.h>
#endif
	])
	AC_CHECK_FUNCS([getprocs64],,,[
#if HAVE_PROCINFO_H
#include <procinfo.h>
#endif
	])
fi


#--------------------------------------------------------------------
# Solaris specific checks
#--------------------------------------------------------------------

if test "x$PLATFORM" = "xSunOS"; then
	AC_CHECK_HEADERS([zone.h])
	AC_CHECK_FUNCS([atomic_inc_32_nv atomic_inc_64_nv atomic_dec_32_nv atomic_dec_64_nv atomic_swap_ptr getzoneid],,,[
#include <sys/atomic.h>
	])
fi


#--------------------------------------------------------------------
# *BSD specific checks
#--------------------------------------------------------------------

AC_CHECK_HEADERS([sys/event.h])
AC_CHECK_LIB(kvm, kvm_open)
AC_CHECK_FUNCS([kvm_openfiles kvm_getswapinfo kqueue])
AC_CHECK_TYPES([struct kinfo_proc2],,,[
#if HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
])


#--------------------------------------------------------------------
# FreeBSD specific checks
#--------------------------------------------------------------------

if test "x$PLATFORM" = "xFreeBSD"; then
	AC_CHECK_HEADERS([net/iso88025.h])
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
	AC_CHECK_DECLS([SIOCGIFDATA],,,[
#if HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
	])
	AC_MSG_CHECKING(if kvm_getprocs requires sizeof kinfo_proc argument)
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
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
#include <sys/sysctl.h>
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
# Minix specific checks
#--------------------------------------------------------------------

if test "x$PLATFORM" = "xMinix"; then
	AC_CHECK_HEADERS([minix/minlib.h])
fi


#--------------------------------------------------------------------
# Check for UNICODE stuff
#--------------------------------------------------------------------

if test "x$BUILD_UNICODE" = "xyes"; then
	CPPFLAGS="$CPPFLAGS -DUNICODE"
fi

AC_CHECK_HEADERS([wchar.h wctype.h iconv.h widec.h])
AC_CHECK_TYPES([wchar_t, wint_t],,,[
#if HAVE_WCHAR_H
#include <wchar.h>
#endif
])
AC_CHECK_SIZEOF(wchar_t,,[
#if HAVE_WCHAR_H
#include <wchar.h>
#endif
])
AC_CHECK_FUNCS([wcslen wcsdup wcsncpy wcstoll wcstoull towupper towlower])
AC_CHECK_FUNCS([wcslwr fputws putws fwide vwscanf wcstok wcslcpy wcslcat])
AC_CHECK_FUNCS([vswscanf vfwscanf wcscasecmp wcsncasecmp wcstombs wcsrtombs])
AC_CHECK_FUNCS([mbstowcs mbsrtowcs wcsftime wcscasestr])
AC_CHECK_FUNCS([wprintf vwprintf swprintf vswprintf fwprintf vfwprintf])

AC_CHECK_DECLS([putws, towupper, towlower],,,[
#if HAVE_WCHAR_H
#include <wchar.h>
#endif
#if HAVE_WCTYPE_H
#include <wctype.h>
#endif
])

if test "x$ac_cv_func_wcsftime" = "xyes"; then
	AC_MSG_CHECKING(whether wcsftime is working correctly)
	AC_RUN_IFELSE(
	  [AC_LANG_PROGRAM([
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_WCHAR_H
#include <wchar.h>
#endif
	     ], [[
		wchar_t buffer[64];
		struct tm *ltm;
		time_t t;

		t = time(NULL);
		ltm = localtime(&t);
		wcsftime(buffer, 64, L"%Y", ltm);
		return wcstol(buffer, NULL, 10) == ltm->tm_year + 1900 ? 0 : 1;
	     ]])
	  ],
	  [ AC_MSG_RESULT(yes)
	    AC_DEFINE(WORKING_WCSFTIME, 1, Define to 1 if wcsftime is working correctly)
	  ],
	  [ AC_MSG_RESULT(no) ],
	  [ AC_MSG_RESULT(no) ]
	)
fi

if test "x$DISABLE_ICONV" != "xyes" -a "x$DIST_BUILD" != "xyes"; then
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

if test "x$ac_found_iconv" = "xyes"; then
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

   AC_MSG_CHECKING(whether iconv supports state reset)
   AC_RUN_IFELSE(
   [AC_LANG_PROGRAM([
#include <stdio.h>
#if HAVE_ICONV_H
#include <iconv.h>
#endif
      ], [
      iconv_t cd = iconv_open("UTF-8","$valid_ucs2_code");
      if (cd == (iconv_t)(-1)) return 1;
      return iconv(cd, NULL, NULL, NULL, NULL) != 0;
      ])
   ],
   [ AC_MSG_RESULT(yes)
      AC_DEFINE(HAVE_ICONV_STATE_RESET, 1, Define to 1 if iconv supports state reset)
   ],
   [ AC_MSG_RESULT(no) ],
   [ AC_MSG_RESULT(no) ]
   )

   # taken from ZSH's configure
   # Check if iconv uses const in prototype declaration
	AC_LANG_PUSH([C++])
	OLD_CXXFLAGS="$CXXFLAGS"
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
	CXXFLAGS="$OLD_CXXFLAGS"
	AC_LANG_POP([C++])
	AC_DEFINE_UNQUOTED([ICONV_CONST], $ac_cv_iconv_const, [Define as const if the declaration of iconv() needs const.])
fi

if test "x$ac_cv_type_wchar_t" = "xyes"; then
	if test $ac_cv_sizeof_wchar_t -eq 2; then
		AC_DEFINE(UNICODE_UCS2, 1, Define to 1 if you have 2-byte wchar_t)
	fi

	if test $ac_cv_sizeof_wchar_t -eq 4; then
		AC_DEFINE(UNICODE_UCS4, 1, Define to 1 if you have 4-byte wchar_t)
	fi
else
	if test "x$BUILD_UNICODE" = "xyes"; then
		AC_MSG_ERROR(wchar_t is required for UNICODE build)
	fi
fi


#--------------------------------------------------------------------
# PCRE & PCRE2
#--------------------------------------------------------------------

HAVE_PCRE="no"
HAVE_PCRE2="no"

if test "x$DIST_BUILD" != "xyes"; then
	AC_CHECK_HEADER(pcre.h,
		[
			AC_DEFINE(HAVE_PCRE_H, 1, [Define to 1 if you have the <pcre.h> header file.])
			HAVE_PCRE="yes"
		],
		[
			AC_CHECK_HEADER(pcre/pcre.h,
				[
					AC_DEFINE(HAVE_PCRE_PCRE_H, 1, [Define to 1 if you have the <pcre/pcre.h> header file.])
					HAVE_PCRE="yes"
				])
		])
	if test "$HAVE_PCRE" = "yes"; then
		AC_CHECK_LIB(pcre, pcre_compile, [PCRE_LIBS="-lpcre"], [HAVE_PCRE="no"])
		if test $ac_cv_sizeof_wchar_t -eq 2; then
			AC_CHECK_LIB(pcre16, pcre16_compile, [PCRE_LIBS="$PCRE_LIBS -lpcre16"], [HAVE_PCRE="no"])
		else
			AC_CHECK_LIB(pcre32, pcre32_compile, [PCRE_LIBS="$PCRE_LIBS -lpcre32"], [HAVE_PCRE="no"])
		fi
	fi
fi

if test "x$DIST_BUILD" != "xyes"; then
	AC_CHECK_HEADER(pcre2.h,
		[
			AC_DEFINE(HAVE_PCRE2_H, 1, [Define to 1 if you have the <pcre2.h> header file.])
			HAVE_PCRE2="yes"
		],
		[
			AC_CHECK_HEADER(pcre2/pcre2.h,
				[
					AC_DEFINE(HAVE_PCRE2_PCRE2_H, 1, [Define to 1 if you have the <pcre2/pcre2.h> header file.])
					HAVE_PCRE2="yes"
				],
                [],
                [
                    #define PCRE2_CODE_UNIT_WIDTH 8
                ])
		],
        [
            #define PCRE2_CODE_UNIT_WIDTH 8
        ])
	if test "$HAVE_PCRE2" = "yes"; then
		AC_CHECK_LIB(pcre2-8, pcre2_compile_8, [PCRE2_LIBS="-lpcre2-8"], [HAVE_PCRE2="no"])
		if test $ac_cv_sizeof_wchar_t -eq 2; then
			AC_CHECK_LIB(pcre2-16, pcre2_compile_16, [PCRE2_LIBS="$PCRE2_LIBS -lpcre2-16"], [HAVE_PCRE2="no"])
		else
			AC_CHECK_LIB(pcre2-32, pcre2_compile_32, [PCRE2_LIBS="$PCRE2_LIBS -lpcre2-32"], [HAVE_PCRE2="no"])
		fi
	fi

	if test "$HAVE_PCRE" = "no" -a "$HAVE_PCRE2" = "no"; then
		AC_MSG_ERROR(Either libpcre or libpcre2 is required)
	fi

	if test "$HAVE_PCRE2" = "yes"; then
		AC_DEFINE(USE_PCRE2, 1, [Define to 1 if PCRE2 is used as regular expression engine])
	fi
fi

#--------------------------------------------------------------------
# MySQL
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "mysql"
if test $? = 0; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LD_RUN_PATH="$LD_RUN_PATH"

	CPPFLAGS="$CPPFLAGS $MYSQL_CPPFLAGS"
	LDFLAGS="$LDFLAGS $MYSQL_LDFLAGS"
	if test "x$MYSQL_LD_RUN_PATH" != "x"; then
		LD_RUN_PATH="${MYSQL_LD_RUN_PATH}${LD_RUN_PATH:+:}${LD_RUN_PATH}"
	fi

	AC_CHECK_HEADER(mysql.h,,AC_MSG_ERROR([Cannot find mysql.h - check your MySQL client installation]))

	AC_CHECK_LIB(mysqlclient, mysql_init, [MYSQL_LIBS="-lmysqlclient"],
		[
 			AC_MSG_ERROR(libmysqlclient is requred for MySQL support)
		])

	OLD_LIBS="$LIBS"
	LIBS="$LIBS $MYSQL_LIBS"

	AC_CHECK_FUNCS(mysql_real_escape_string)

	AC_CHECK_TYPES([my_bool],,,
		[
#include <mysql.h>
		])

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LD_RUN_PATH="$OLD_LD_RUN_PATH"
	LIBS="$OLD_LIBS"
fi


#--------------------------------------------------------------------
# MariaDB
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "mariadb"
if test $? = 0; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LD_RUN_PATH="$LD_RUN_PATH"

	CPPFLAGS="$CPPFLAGS $MARIADB_CPPFLAGS"
	LDFLAGS="$LDFLAGS $MARIADB_LDFLAGS"
	if test "x$MARIADB_LD_RUN_PATH" != "x"; then
		LD_RUN_PATH="${MARIADB_LD_RUN_PATH}${LD_RUN_PATH:+:}${LD_RUN_PATH}"
	fi

	AC_CHECK_HEADER(mysql.h,,AC_MSG_ERROR([Cannot find mysql.h - check your MariaDB client installation]))

 	AC_CHECK_LIB(mariadb, mysql_init, [MARIADB_LIBS="-lmariadb"],
		[
 			AC_MSG_ERROR(libmariadb is requred for MariaDB support)
		])

	AC_CHECK_DECLS([MYSQL_OPT_CONNECT_ATTRS, MYSQL_OPT_SSL_ENFORCE, mysql_get_optionv],,,[
#include <mysql.h>
		])

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LD_RUN_PATH="$OLD_LD_RUN_PATH"
fi


#--------------------------------------------------------------------
# PostgreSQL
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "pgsql"
if test $? = 0; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LD_RUN_PATH="$LD_RUN_PATH"

	CPPFLAGS="$CPPFLAGS $PGSQL_CPPFLAGS"
	LDFLAGS="$LDFLAGS $PGSQL_LDFLAGS"
	if test "x$PGSQL_LD_RUN_PATH" != "x"; then
		LD_RUN_PATH="${PGSQL_LD_RUN_PATH}${LD_RUN_PATH:+:}${LD_RUN_PATH}"
	fi

	AC_CHECK_HEADER(libpq-fe.h,,AC_MSG_ERROR([Cannot find libpq-fe.h - check your PostgreSQL client installation]))

	AC_CHECK_DECLS([PGRES_SINGLE_TUPLE],,,[
#include <libpq-fe.h>
	])

	AC_CHECK_LIB(m, floor)
	AC_CHECK_LIB(pq, PQconnectdb, [PGSQL_LIBS="-lpq"], [
		if test "x$PLATFORM" = "xSunOS"; then
			if test -d "$PGSQL_LD_RUN_PATH/sparcv9"; then
				PGSQL_LDFLAGS="$PGSQL_LDFLAGS/sparcv9"
			else
				PGSQL_LDFLAGS="$PGSQL_LDFLAGS/64"
			fi
			LDFLAGS="$OLD_LDFLAGS $PGSQL_LDFLAGS"
			if test "x$PGSQL_LD_RUN_PATH" != "x"; then
				PGSQL_LD_RUN_PATH="$PGSQL_LD_RUN_PATH/sparcv9"
				LD_RUN_PATH="${PGSQL_LD_RUN_PATH}${OLD_LD_RUN_PATH:+:}${OLD_LD_RUN_PATH}"
			fi
			unset ac_cv_lib_pq_PQconnectdb
			AC_CHECK_LIB(pq, PQconnectdb, [PGSQL_LIBS="-lpq"], [
				AC_MSG_ERROR(libpq is required for PostgreSQL support)])        
		else
			AC_MSG_ERROR(libpq is required for PostgreSQL support)
		fi
	])

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LD_RUN_PATH="$OLD_LD_RUN_PATH"
fi


#--------------------------------------------------------------------
# ODBC
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "odbc"
if test $? = 0; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LD_RUN_PATH="$LD_RUN_PATH"

	CPPFLAGS="$CPPFLAGS $ODBC_CPPFLAGS"
	LDFLAGS="$LDFLAGS $ODBC_LDFLAGS"
	if test "x$ODBC_LD_RUN_PATH" != "x"; then
		LD_RUN_PATH="${ODBC_LD_RUN_PATH}${LD_RUN_PATH:+:}${LD_RUN_PATH}"
	fi

	AC_CHECK_LIB(odbc, SQLAllocHandle, [ODBC_LIBS="-lodbc"],
		[AC_MSG_ERROR(libodbc is required for ODBC support)])
	if test "x$BUILD_STATIC_AGENT" = "xyes"; then
		SUBAGENT_LIBS="$SUBAGENT_LIBS $ODBC_LIBS"
		AC_CHECK_LIB(ltdl, lt_dlopen, [SUBAGENT_LIBS="$SUBAGENT_LIBS -lltdl"], [])
	fi

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LD_RUN_PATH="$OLD_LD_RUN_PATH"
fi


#--------------------------------------------------------------------
# Oracle
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "oracle"
if test $? = 0; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LD_RUN_PATH="$LD_RUN_PATH"

	CPPFLAGS="$CPPFLAGS $ORACLE_CPPFLAGS"
	LDFLAGS="$LDFLAGS $ORACLE_LDFLAGS"
	if test "x$ORACLE_LD_RUN_PATH" != "x"; then
		LD_RUN_PATH="${ORACLE_LD_RUN_PATH}${LD_RUN_PATH:+:}${LD_RUN_PATH}"
	fi

	AC_CHECK_HEADER(oci.h,,AC_MSG_ERROR([*** Cannot find oci.h - check your Oracle client installation ***]))
	AC_CHECK_LIB(clntsh, OCIHandleAlloc, [ORACLE_LIBS="-lclntsh"],
		[AC_MSG_ERROR(libclntsh is required for Oracle support)])
	AC_CHECK_LIB(rt, sem_destroy, [ORACLE_LIBS="$ORACLE_LIBS -lrt"], [])
	if test "x$BUILD_STATIC_AGENT" = "xyes"; then
		SUBAGENT_LIBS="$SUBAGENT_LIBS $ORACLE_LIBS"
	fi

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LD_RUN_PATH="$OLD_LD_RUN_PATH"
fi

#--------------------------------------------------------------------
# DB2
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "db2"
if test $? = 0; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LD_RUN_PATH="$LD_RUN_PATH"

	CPPFLAGS="$CPPFLAGS $DB2_CPPFLAGS"
	LDFLAGS="$LDFLAGS $DB2_LDFLAGS"
	if test "x$DB2_LD_RUN_PATH" != "x"; then
		LD_RUN_PATH="${DB2_LD_RUN_PATH}${LD_RUN_PATH:+:}${LD_RUN_PATH}"
	fi

	AC_CHECK_HEADER(sql.h,,AC_MSG_ERROR([*** Cannot find sql.h - check your DB2 client installation ***]))
	AC_CHECK_LIB(db2, SQLAllocHandle, [DB2_LIBS="-ldb2"],
		[AC_MSG_ERROR(libdb2 is required for DB2 support)])
	if test "x$BUILD_STATIC_AGENT" = "xyes"; then
		SUBAGENT_LIBS="$SUBAGENT_LIBS $DB2_LIBS"
	fi

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LD_RUN_PATH="$OLD_LD_RUN_PATH"
fi

#--------------------------------------------------------------------
# Informix
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "informix"
if test $? = 0; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LD_RUN_PATH="$LD_RUN_PATH"

	CPPFLAGS="$CPPFLAGS $INFORMIX_CPPFLAGS"
	LDFLAGS="$LDFLAGS $INFORMIX_LDFLAGS"
	if test "x$INFORMIX_LD_RUN_PATH" != "x"; then
		LD_RUN_PATH="${INFORMIX_LD_RUN_PATH}${LD_RUN_PATH:+:}${LD_RUN_PATH}"
	fi

	AC_CHECK_HEADER(infxcli.h,,AC_MSG_ERROR([*** Cannot find infxcli.h - check your Informix client installation ***]))
	AC_CHECK_LIB(thcli, SQLAllocHandle, [INFORMIX_LIBS="-lthcli"],
		[AC_MSG_ERROR(libthcli is required for Informix support)])
	if test "x$BUILD_STATIC_AGENT" = "xyes"; then
		SUBAGENT_LIBS="$SUBAGENT_LIBS $INFORMIX_LIBS"
	fi

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LD_RUN_PATH="$OLD_LD_RUN_PATH"
fi


#--------------------------------------------------------------------
# Java
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "java"
if test $? = 0; then
	if test "x$JDK_LOCATION" = "x"; then
		JDK_LOCATION="/usr/java"
		AC_DEFINE_UNQUOTED([JDK_LOCATION],["$JDK_LOCATION"],[JDK location])
	fi

	LD_RUN_PATH="${JDK_LOCATION}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"   
	LDFLAGS="$LDFLAGS -L${JDK_LOCATION}/lib"
	CPPFLAGS="$CPPFLAGS -I${JDK_LOCATION}/include"
	if test -d "${JDK_LOCATION}/include/aix"; then
		CPPFLAGS="$CPPFLAGS -I${JDK_LOCATION}/include/aix"
	fi
	if test -d "${JDK_LOCATION}/include/darwin"; then
		CPPFLAGS="$CPPFLAGS -I${JDK_LOCATION}/include/darwin"
	fi
	if test -d "${JDK_LOCATION}/include/freebsd"; then
		CPPFLAGS="$CPPFLAGS -I${JDK_LOCATION}/include/freebsd"
	fi
	if test -d "${JDK_LOCATION}/include/linux"; then
		CPPFLAGS="$CPPFLAGS -I${JDK_LOCATION}/include/linux"
	fi
	if test -d "${JDK_LOCATION}/include/openbsd"; then
		CPPFLAGS="$CPPFLAGS -I${JDK_LOCATION}/include/openbsd"
	fi
	if test -d "${JDK_LOCATION}/include/solaris"; then
		CPPFLAGS="$CPPFLAGS -I${JDK_LOCATION}/include/solaris"
	fi
	AC_CHECK_HEADER(jni.h,,AC_MSG_ERROR([*** Cannot find jni.h - check your JDK installation ***]))
fi


#--------------------------------------------------------------------
# microhttpd
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "server"
if test $? = 0 -o "x$REQUIRE_MICROHTTPD" = "xyes"; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"

	CPPFLAGS="$CPPFLAGS $MICROHTTPD_CPPFLAGS"
	LDFLAGS="$LDFLAGS $MICROHTTPD_LDFLAGS"

	MICROHTTPD_SUPPORT="yes"
	AC_CHECK_HEADER(microhttpd.h, [], [
		if test "x$REQUIRE_MICROHTTPD" = "xyes"; then
			AC_MSG_ERROR([*** Cannot find microhttpd.h - check your libmicrohttpd installation ***])
		else
			MICROHTTPD_SUPPORT="no"
		fi
	])
	AC_CHECK_LIB(microhttpd, MHD_start_daemon, [ MICROHTTPD_LIBS="-lmicrohttpd" ], [
		if test "x$REQUIRE_MICROHTTPD" = "xyes"; then
			AC_MSG_ERROR([*** libmicrohttpd is missing or too old - check your libmicrohttpd installation ***])
		else
			MICROHTTPD_SUPPORT="no"
		fi
	])

	if test "x$MICROHTTPD_SUPPORT" = "xyes"; then
		AC_DEFINE(WITH_MICROHTTPD, 1, Define to 1 if microhttpd is supported)
		SERVER_COMPONENTS="webapi $SERVER_COMPONENTS"
		HDLINK_DIRS="$HDLINK_DIRS jira"
	else
		AC_MSG_ERROR([*** libmicrohttpd is required ***])
	fi

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
fi


#--------------------------------------------------------------------
# Tuxedo
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "tuxedo"
if test $? = 0; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"
	OLD_LD_RUN_PATH="$LD_RUN_PATH"

	if test "x$TUXEDO_BASE" != "x"; then
		TUXEDO_CPPFLAGS="-I$TUXEDO_BASE/include"
		TUXEDO_LDFLAGS="-L$TUXEDO_BASE/lib"
		CPPFLAGS="$CPPFLAGS $TUXEDO_CPPFLAGS"
		LDFLAGS="$LDFLAGS $TUXEDO_LDFLAGS"
		LD_RUN_PATH="${TUXEDO_BASE}/lib${LD_RUN_PATH:+:}${LD_RUN_PATH}"
	fi
	
	AC_CHECK_HEADER(atmi.h,,AC_MSG_ERROR([*** Cannot find atmi.h - check your Tuxedo installation ***]))

	# https://community.oracle.com/thread/2346582
	if test "x$PLATFORM" = "xLinux"; then
		TUXEDO_LDFLAGS="$TUXEDO_LDFLAGS -Xlinker --no-as-needed"
		LDFLAGS="$LDFLAGS -Xlinker --no-as-needed"
	fi

	if test "$PLATFORM" = "SunOS"; then
		AC_CHECK_LIB(rt, nanosleep, [ TUXEDO_LIBS="$TUXEDO_LIBS -lrt" ])
	fi

	if test "$PLATFORM" = "AIX"; then
		AC_MSG_CHECKING(for xa_switchp in libengine)
		OLD_LIBS_LIBENGINE="$LIBS"
		LIBS="$LIBS -lengine"
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[ extern char *xa_switchp; ]], [[ *xa_switchp = 0; ]])],
			[ 
				AC_MSG_RESULT(yes) 
				TUXEDO_LIBS="$TUXEDO_LIBS -lengine"
			],
         		[ AC_MSG_RESULT(no) ])
		LIBS="$OLD_LIBS_LIBENGINE"
	fi

	LIBS="$LIBS $TUXEDO_LIBS"
	AC_CHECK_LIB(tux, tpcall, [ TUXEDO_LIBS="-ltux -lfml32 $TUXEDO_LIBS" ],
		[
		 	# Assume we are building for Enduro/X
			AC_CHECK_LIB(rt, mq_open, [ TUXEDO_LIBS="$TUXEDO_LIBS -lrt" ])
			AC_CHECK_LIB(atmi, tpcall,
				[ TUXEDO_LIBS="-latmi -lubf -lnstd $TUXEDO_LIBS" ],
				[ AC_MSG_ERROR(libtux or libatmi is required for Tuxedo support)],
				[ -lubf -lnstd $TUXEDO_LIBS ])
		])

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
	LD_RUN_PATH="$OLD_LD_RUN_PATH"

	if test "x$PLATFORM" = "xAIX"; then
		if test -e $TUXEDO_BASE/lib/tpinit.o; then
			TUXEDO_TPINIT="$TUXEDO_BASE/lib/tpinit.o"
		fi
	fi
fi


#--------------------------------------------------------------------
# ZeroMQ
#--------------------------------------------------------------------

if test "x$DISABLE_ZERMQ" != "xyes"; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"

	if test "x$ZEROMQ_BASE" != "x"; then
		ZEROMQ_CPPFLAGS="-I$ZEROMQ_BASE/include"
		ZEROMQ_LDFLAGS="-L$ZEROMQ_BASE/lib"
		CPPFLAGS="$CPPFLAGS $ZEROMQ_CPPFLAGS"
		LDFLAGS="$LDFLAGS $ZEROMQ_LDFLAGS"
	fi
	
   ZEROMQ_SUPPORT="yes"
	AC_CHECK_HEADER(zmq.h,,[
		if test "x$REQUIRE_ZEROMQ" = "xyes"; then
			AC_MSG_ERROR([*** Cannot find zmq.h - check your ZeroMQ installation ***])
		else
			ZEROMQ_SUPPORT="no"
		fi
	])
	AC_CHECK_LIB(zmq, zmq_ctx_new, [], [
		if test "x$REQUIRE_ZEROMQ" = "xyes"; then
			AC_MSG_ERROR([*** libzmq is missing or too old - check your ZeroMQ installation ***])
		else
			ZEROMQ_SUPPORT="no"
		fi
	])

	if test "x$ZEROMQ_SUPPORT" = "xyes"; then
		AC_DEFINE(WITH_ZMQ, 1, Define to 1 if ZeroMQ is supported)
	   COMPONENTS="$COMPONENTS zeromq"
	fi

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
fi


#--------------------------------------------------------------------
# MQTT
#--------------------------------------------------------------------

if test "x$DISABLE_MQTT" != "xyes" -a "x$DIST_BUILD" != "xyes"; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"

	if test "x$MOSQUITTO_BASE" != "x"; then
		MQTT_CPPFLAGS="-I$MOSQUITTO_BASE/include"
		MQTT_LDFLAGS="-L$MOSQUITTO_BASE/lib"
		CPPFLAGS="$CPPFLAGS $MQTT_CPPFLAGS"
		LDFLAGS="$LDFLAGS $MQTT_LDFLAGS"
	fi

	AC_CHECK_HEADER(mosquitto.h,[
		AC_CHECK_LIB(mosquitto, mosquitto_lib_init, [
			MQTT_LIBS="-lmosquitto"
			MQTT_SUPPORT="yes"
			AC_DEFINE(WITH_MQTT, 1, Define to 1 if MQTT is supported)
			SUBAGENT_DIRS="$SUBAGENT_DIRS mqtt"
			NCDRV_MODULES="$NCDRV_MODULES mqtt"

			LIBS="$LIBS $MQTT_LIBS"
			AC_CHECK_FUNCS([mosquitto_threaded_set])
		],[
			if test "x$REQUIRE_MQTT" = "xyes"; then
				AC_MSG_ERROR([*** libmosquitto is missing - check your libmosquitto installation ***])
			fi
		])
	],[
		if test "x$REQUIRE_MQTT" = "xyes"; then
			AC_MSG_ERROR([*** Cannot find mosquitto.h - check your libmosquitto installation ***])
		fi
	])

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
fi


#--------------------------------------------------------------------
# Python
#--------------------------------------------------------------------

if test "x$PYTHON_SUPPORT" = "xyes"; then
	PYTHON_VERSIONS="3.8 3.7 3.6m 3.6 3.5m 3.5 3m 3"

	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"

	if test "x$PYTHON_BASE" != "x"; then
		BIN_PREFIX="$PYTHON_BASE/bin/"
	fi

	PYTHON_VERSION=""
	for PV in $PYTHON_VERSIONS; do
		PY_CONFIG="${BIN_PREFIX}python${PV}-config"
		"$PY_CONFIG" --prefix >/dev/null
		if test $? -eq 0; then
			PYTHON_VERSION="$PV"
			break
		fi
	done

	if test "x$PYTHON_VERSION" != "x"; then
		if $PY_CONFIG --help | grep embed >/dev/null; then
			PY_EMBED="--embed"
		fi
		PYTHON_CPPFLAGS=`$PY_CONFIG --includes`
		PYTHON_LDFLAGS=`$PY_CONFIG --ldflags`
		PYTHON_LIBS=`$PY_CONFIG --libs $PY_EMBED`

		CPPFLAGS="$CPPFLAGS $PYTHON_CPPFLAGS"
		LDFLAGS="$LDFLAGS $PYTHON_LDFLAGS"
		LIBS="$LIBS $PYTHON_LIBS"
	
		AC_CHECK_HEADER(Python.h,,[
			AC_MSG_ERROR([*** Cannot find Python.h - check your libpython installation or disable Python support with --without-python ***])
		])
		AC_CHECK_FUNC(Py_Initialize, , [
			AC_MSG_ERROR([*** Cannot find Py_Initialize function - check your libpython installation or disable Python support with --without-python ***])
		])
    
		AC_DEFINE(WITH_PYTHON, 1, Define to 1 if Python is supported)
	else
		AC_MSG_ERROR([*** Cannot find Python configuration tool - check your libpython installation or disable Python support with --without-python ***])
	fi

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
fi


#--------------------------------------------------------------------
# MongoDB
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "mongodb"
if test $? = 0; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"

	if test "x$MONGODB_BASE" != "x"; then
		MONGODB_CPPFLAGS="-I$MONGODB_BASE/include/libmongoc-1.0/ -I$MONGODB_BASE/include/libbson-1.0/"
		MONGODB_LDFLAGS="-L$MONGODB_BASE/lib"
		CPPFLAGS="$CPPFLAGS $MONGODB_CPPFLAGS"
		LDFLAGS="$LDFLAGS $MONGODB_LDFLAGS"
	else
		if test -d /usr/include/libmongoc-1.0; then
			MONGODB_CPPFLAGS="-I/usr/include/libmongoc-1.0"
		else
			if test -d /usr/local/include/libmongoc-1.0; then
				MONGODB_CPPFLAGS="-I/usr/local/include/libmongoc-1.0"
				MONGODB_LDFLAGS="-L/usr/local/lib"
			fi
		fi
		if test -d /usr/include/libbson-1.0; then
			MONGODB_CPPFLAGS="$MONGODB_CPPFLAGS -I/usr/include/libbson-1.0"
		else
			if test -d /usr/local/include/libbson-1.0; then
				MONGODB_CPPFLAGS="$MONGODB_CPPFLAGS -I/usr/local/include/libbson-1.0"
				MONGODB_LDFLAGS="-L/usr/local/lib"
			fi
		fi
		CPPFLAGS="$CPPFLAGS $MONGODB_CPPFLAGS"
		LDFLAGS="$LDFLAGS $MONGODB_LDFLAGS"
    	fi

	AC_CHECK_HEADER(mongoc.h,,AC_MSG_ERROR([*** Cannot find mongoc.h - check your MongoDB installation ***]))
	AC_CHECK_LIB(mongoc-1.0, mongoc_client_get_database, [ MONGODB_LIBS="$MONGODB_LIBS -lmongoc-1.0" ])
	AC_CHECK_LIB(bson-1.0, nanosleep, [ MONGODB_LIBS="$MONGODB_LIBS -lbson-1.0" ])
	LIBS="$LIBS $MONGODB_LIBS"

	AC_CHECK_FUNCS([mongoc_client_get_database_names_with_opts])

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
fi


#--------------------------------------------------------------------
# Redis
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "redis"
if test $? = 0; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"

	if test "x$REDIS_CPPFLAGS" = "x"; then
		# Check standard locations
		if test -d /usr/include/hiredis; then
			REDIS_CPPFLAGS="-I/usr/include"
		elif test -d /usr/local/include/hiredis; then
			REDIS_CPPFLAGS="-I/usr/local/include"
			REDIS_LDFLAGS="-L/usr/local/lib"
		elif test -d /opt/homebrew/include/hiredis; then
			REDIS_CPPFLAGS="-I/opt/homebrew/include"
			REDIS_LDFLAGS="-L/opt/homebrew/lib"
		fi
	fi

	CPPFLAGS="$CPPFLAGS $REDIS_CPPFLAGS"
	LDFLAGS="$LDFLAGS $REDIS_LDFLAGS"

	AC_CHECK_HEADER(hiredis/hiredis.h,,AC_MSG_ERROR([*** Cannot find hiredis/hiredis.h - check your hiredis installation or use --with-redis=DIR ***]))
	AC_CHECK_LIB(hiredis, redisConnect, [ REDIS_LIBS="-lhiredis" ], AC_MSG_ERROR([*** Cannot find hiredis library ***]))

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
fi

AC_SUBST(REDIS_CPPFLAGS)
AC_SUBST(REDIS_LDFLAGS)
AC_SUBST(REDIS_LIBS)


#--------------------------------------------------------------------
# Asterisk
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "asterisk"
if test $? = 0; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"

	AC_CHECK_HEADER(eXosip2/eXosip.h,,AC_MSG_ERROR([*** Cannot find eXosip.h - check your libeXosip2 installation ***]))
	AC_CHECK_LIB(eXosip2, eXosip_malloc, [ ASTERISK_LIBS="$ASTERISK_LIBS -leXosip2" ])

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
fi


#--------------------------------------------------------------------
# VMGR
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "vmgr"
if test $? = 0; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"

	if test "x$LIBVIRT_BASE" != "x"; then
		VMGR_CPPFLAGS="-I$LIBVIRT_BASE/include"
		VMGR_LDFLAGS="-L$LIBVIRT_BASE/lib"
		CPPFLAGS="$CPPFLAGS $VMGR_CPPFLAGS"
		LDFLAGS="$LDFLAGS $VMGR_LDFLAGS"
	else
        	VMGR_CPPFLAGS=""
		VMGR_LDFLAGS=""
		CPPFLAGS="$CPPFLAGS $VMGR_CPPFLAGS"
		LDFLAGS="$LDFLAGS $VMGR_LDFLAGS"
    	fi
	
	AC_CHECK_HEADER(libvirt/libvirt.h,,AC_MSG_ERROR([*** Cannot find libvirt.h - check your libvirt installation ***]))
	AC_CHECK_LIB(virt, virConnectGetLibVersion, [ VMGR_LIBS="$VMGR_LIBS -lvirt" ])
	LIBS="$LIBS $VMGR_LIBS"

	AC_CHECK_FUNCS([virDomainGetTime])

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
fi


#--------------------------------------------------------------------
# XEN
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "xen"
if test $? = 0; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"

	AC_CHECK_HEADER(libxl.h,,AC_MSG_ERROR([*** Cannot find libxl.h - check your libxen installation ***]))
	AC_CHECK_LIB(xenlight, libxl_ctx_alloc, [ XEN_LIBS="$XEN_LIBS -lxenlight" ])
	AC_CHECK_LIB(xenctrl, xc_vcpu_getinfo, [ XEN_LIBS="$XEN_LIBS -lxenctrl" ])
	AC_CHECK_DECLS([ERROR_DOMAIN_NOTFOUND],,,[
#include <libxl.h>
	])

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
fi


#--------------------------------------------------------------------
# jemalloc
#--------------------------------------------------------------------

if test "x$JEMALLOC_REQUIRED" = "xyes"; then
	if test "x$ENABLE_SANITIZER" = "xyes"; then
		AC_MSG_ERROR(Sanitizer and jemalloc are mutually exclusive)
	fi
	AC_CHECK_HEADERS([jemalloc/jemalloc.h],
		[
			rootdir=`pwd`
			if test "x$PLATFORM" = "xAIX"; then
				EXEC_LDFLAGS=""
				AC_CHECK_LIB(jemalloc, je_malloc,
					[ 
						LIBS="$LIBS -ljemalloc"
						AC_DEFINE(WITH_JEMALLOC,1,Define to 1 if libjemalloc is used)
					],
					[
						AC_MSG_ERROR(Cannot find usable libjemalloc)
					])
			else
		 		AC_CHECK_LIB(rt, clock_gettime)
				AC_CHECK_LIB(jemalloc_pic, malloc,
					[ 
						EXEC_LDFLAGS="$EXEC_LDFLAGS -export-symbols $rootdir/jemalloc.sym"
						EXEC_LIBS="$EXEC_LIBS -ljemalloc_pic"
						AC_DEFINE(WITH_JEMALLOC,1,Define to 1 if libjemalloc is used)
					],
					[
						AC_MSG_ERROR(Cannot find usable libjemalloc)
					])
			fi

		],
		[
			AC_MSG_ERROR(jemalloc.h not found)
		])
fi


#--------------------------------------------------------------------
# Sanitizer
#--------------------------------------------------------------------

if test "x$ENABLE_SANITIZER" = "xyes"; then
	AC_CPP_CHECK_OPTION(-fsanitize=address)
	AC_CPP_CHECK_OPTION(-fsanitize=leak)
	AC_CPP_CHECK_OPTION(-fsanitize=undefined)
	AC_CPP_CHECK_OPTION(-fstack-protector)
	AC_CPP_CHECK_OPTION(-fno-omit-frame-pointer)
	AC_CPP_CHECK_OPTION(-funwind-tables)
	AC_CPP_CHECK_OPTION(-fasynchronous-unwind-tables)
	AC_LD_CHECK_OPTION(-fsanitize=address)
	AC_LD_CHECK_OPTION(-fsanitize=leak)
	AC_LD_CHECK_OPTION(-fsanitize=undefined)
	AC_DEFINE(WITH_ADDRESS_SANITIZER,1,Define to 1 if address sanitizer is enabled)
fi


#--------------------------------------------------------------------
# protobuf
#--------------------------------------------------------------------

if test "x$REQUIRE_PROTOBUF" = "xyes"; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"

	CPPFLAGS="$CPPFLAGS $PROTOBUF_CPPFLAGS $ABSEIL_CPPFLAGS"
	LDFLAGS="$LDFLAGS $PROTOBUF_LDFLAGS $ABSEIL_LDFLAGS"

	AC_CHECK_PROG(PROTOC, protoc, protoc)
	if test "x$PROTOC" = "x"; then
		if test "x$REQUIRE_PROTOBUF" = "xyes"; then
			AC_MSG_ERROR([*** protoc compiler not found - check your protobuf installation ***])
		else
			COMPONENTS=`echo "$COMPONENTS" | sed 's/protobuf//'`
		fi
	else
		PROTOBUF_SUPPORT="yes"
		AC_LANG_PUSH(C++)
		AC_CHECK_HEADER(google/protobuf/message.h, [], [
			if test "x$REQUIRE_PROTOBUF" = "xyes"; then
				AC_MSG_ERROR([*** Cannot find google/protobuf/message.h - check your protobuf installation ***])
			else
				PROTOBUF_SUPPORT="no"
			fi
		])
		AC_CHECK_LIB(protobuf, main, [ PROTOBUF_LIBS="-lprotobuf" ], [
			if test "x$REQUIRE_PROTOBUF" = "xyes"; then
				AC_MSG_ERROR([*** libprotobuf is missing - check your protobuf installation ***])
			else
				PROTOBUF_SUPPORT="no"
			fi
		])
		AC_LANG_POP(C++)

		if test "x$PROTOBUF_SUPPORT" = "xyes"; then
			AC_DEFINE(WITH_PROTOBUF, 1, Define to 1 if protobuf is supported)
		else
			if test "x$REQUIRE_PROTOBUF" = "xyes"; then
				AC_MSG_ERROR([*** libprotobuf is required ***])
			fi
		fi
	fi

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
fi


#--------------------------------------------------------------------
# snappy
#--------------------------------------------------------------------

if test "x$REQUIRE_SNAPPY" = "xyes"; then
	OLD_CPPFLAGS="$CPPFLAGS"
	OLD_LDFLAGS="$LDFLAGS"
	OLD_LIBS="$LIBS"

	CPPFLAGS="$CPPFLAGS $SNAPPY_CPPFLAGS"
	LDFLAGS="$LDFLAGS $SNAPPY_LDFLAGS"

	SNAPPY_SUPPORT="yes"
	AC_LANG_PUSH(C++)
	AC_CHECK_HEADER(snappy.h, [], [
		if test "x$REQUIRE_SNAPPY" = "xyes"; then
			AC_MSG_ERROR([*** Cannot find snappy.h - check your libsnappy installation ***])
		else
			SNAPPY_SUPPORT="no"
		fi
	])
	AC_CHECK_LIB(snappy, snappy_compress, [ SNAPPY_LIBS="-lsnappy" ], [
		if test "x$REQUIRE_SNAPPY" = "xyes"; then
			AC_MSG_ERROR([*** libsnappy is missing - check your libsnappy installation ***])
		else
			SNAPPY_SUPPORT="no"
		fi
	])
	AC_LANG_POP(C++)

	if test "x$SNAPPY_SUPPORT" = "xyes"; then
		AC_DEFINE(WITH_SNAPPY, 1, Define to 1 if snappy is supported)
	else
		if test "x$REQUIRE_SNAPPY" = "xyes"; then
			AC_MSG_ERROR([*** libsnappy is required ***])
		fi
	fi

	CPPFLAGS="$OLD_CPPFLAGS"
	LDFLAGS="$OLD_LDFLAGS"
	LIBS="$OLD_LIBS"
fi


#--------------------------------------------------------------------
# Prometheus
#--------------------------------------------------------------------

check_substr "$COMPONENTS" "prometheus"
if test $? = 0; then
	AC_DEFINE(WITH_PROMETHEUS, 1, Define to 1 if Prometheus remote write support is enabled)
	PROMETHEUS_LIBS="$MICROHTTPD_LIBS $PROTOBUF_LIBS $SNAPPY_LIBS"
	SUBAGENT_DIRS="$SUBAGENT_DIRS prometheus"
fi


#--------------------------------------------------------------------
# Other settings
#--------------------------------------------------------------------

if test "x$RUNTIME_PREFIX" = "x"; then
	NETXMS_PREFIX='${prefix}'
	NETXMS_DATADIR='${pkgdatadir}'
	NETXMS_BINDIR='${bindir}'
	NETXMS_LIBDIR='${libdir}'
	NETXMS_PKGLIBDIR='${pkglibdir}'
	NETXMS_STATEDIR='${localstatedir}/lib/netxms'
	NETXMS_SYSCONFDIR='${sysconfdir}'
        if test "x${BUILD_UNICODE}" = "xyes"; then
		CPPFLAGS="$CPPFLAGS -DPREFIX=L\\\"\${prefix}\\\" -DDATADIR=L\\\"\${pkgdatadir}\\\" -DBINDIR=L\\\"\${bindir}\\\" -DLIBDIR=L\\\"\${libdir}\\\" -DPKGLIBDIR=L\\\"\${pkglibdir}\\\" -DSTATEDIR=L\\\"\${localstatedir}/lib/netxms\\\" -DSYSCONFDIR=L\\\"\${sysconfdir}\\\""
	else
		CPPFLAGS="$CPPFLAGS -DPREFIX=\\\"\${prefix}\\\" -DDATADIR=\\\"\${pkgdatadir}\\\" -DBINDIR=\\\"\${bindir}\\\" -DLIBDIR=\\\"\${libdir}\\\" -DPKGLIBDIR=\\\"\${pkglibdir}\\\" -DSTATEDIR=\\\"\${localstatedir}/lib/netxms\\\" -DSYSCONFDIR=\\\"\${sysconfdir}\\\""
	fi
else
	NETXMS_PREFIX="$RUNTIME_PREFIX"
	NETXMS_DATADIR="$RUNTIME_PREFIX/share/netxms"
	NETXMS_BINDIR="$RUNTIME_PREFIX/bin"
	NETXMS_LIBDIR="$RUNTIME_PREFIX/lib"
	NETXMS_PKGLIBDIR="$RUNTIME_PREFIX/lib/netxms"
	NETXMS_STATEDIR="$RUNTIME_STATE_DIR/lib/netxms"
	NETXMS_SYSCONFDIR="$RUNTIME_PREFIX/etc"
	if test "x${BUILD_UNICODE}" = "xyes"; then
		CPPFLAGS="$CPPFLAGS -DPREFIX=L\\\"$RUNTIME_PREFIX\\\" -DDATADIR=L\\\"$RUNTIME_PREFIX/share/netxms\\\" -DBINDIR=L\\\"$RUNTIME_PREFIX/bin\\\" -DLIBDIR=L\\\"$RUNTIME_PREFIX/lib\\\" -DPKGLIBDIR=L\\\"$RUNTIME_PREFIX/lib/netxms\\\" -DSTATEDIR=L\\\"$RUNTIME_STATE_DIR/lib/netxms\\\" -DSYSCONFDIR=L\\\"$RUNTIME_PREFIX/etc\\\""
	else
		CPPFLAGS="$CPPFLAGS -DPREFIX=\\\"$RUNTIME_PREFIX\\\" -DDATADIR=\\\"$RUNTIME_PREFIX/share/netxms\\\" -DBINDIR=\\\"$RUNTIME_PREFIX/bin\\\" -DLIBDIR=\\\"$RUNTIME_PREFIX/lib\\\" -DPKGLIBDIR=\\\"$RUNTIME_PREFIX/lib/netxms\\\" -DSTATEDIR=\\\"$RUNTIME_STATE_DIR/lib/netxms\\\" -DSYSCONFDIR=\\\"$RUNTIME_PREFIX/etc\\\""
	fi
	LDFLAGS_SUFFIX="$LDFLAGS_SUFFIX -R $RUNTIME_PREFIX/lib"
fi

LDFLAGS="$LDFLAGS_PREFIX $LDFLAGS $PTHREAD_LDFLAGS $LDFLAGS_SUFFIX"
if test "x$HAVE_PCRE2" = "xyes"; then
	LIBS="$LIBS $PCRE2_LIBS"
else
	LIBS="$LIBS $PCRE_LIBS"
fi
if test "x${RELEASE_BUILD}" = "xyes"; then
	NETXMS_JAR_VERSION="$PACKAGE_VERSION"
else
	NETXMS_JAR_VERSION=`echo $PACKAGE_VERSION | cut -d . -f 1,2`-SNAPSHOT
fi
if test "x${BUILD_UNICODE}" = "xyes"; then
	CPPFLAGS="$CPPFLAGS -DNETXMS_JAR_VERSION=L\\\"$NETXMS_JAR_VERSION\\\""
else
	CPPFLAGS="$CPPFLAGS -DNETXMS_JAR_VERSION=\\\"$NETXMS_JAR_VERSION\\\""
fi

if test "x${ENABLE_WERROR}" = "xyes"; then
	CPPFLAGS="$CPPFLAGS -Werror"
fi

if test "x$PLATFORM" = "xDarwin"; then
	SHLIB_SUFFIX=".so"
	SYSTEM_SHLIB_SUFFIX=".dylib"
else
	if test "x$shrext_cmds" != "x"; then
		eval SHLIB_SUFFIX=$shrext_cmds
	else
		SHLIB_SUFFIX=".so"
	fi
	SYSTEM_SHLIB_SUFFIX="$SHLIB_SUFFIX"
fi
AC_DEFINE_UNQUOTED([SHLIB_SUFFIX], _T("$SHLIB_SUFFIX"), [Define to shared library suffix used by NetXMS libraries])
AC_DEFINE_UNQUOTED([SHLIB_SUFFIX_A], "$SHLIB_SUFFIX", [Define to shared library suffix used by NetXMS libraries])
AC_DEFINE_UNQUOTED([SYSTEM_SHLIB_SUFFIX], _T("$SYSTEM_SHLIB_SUFFIX"), [Define to platform's shared library suffix])
AC_DEFINE_UNQUOTED([SYSTEM_SHLIB_SUFFIX_A], "$SYSTEM_SHLIB_SUFFIX", [Define to platform's shared library suffix])

if test "x$ENABLE_DEBUG" = "xno"; then
	CPPFLAGS="$CPPFLAGS -DNDEBUG"
fi

if test "x$CXX" = "xCC" ; then
	# The following is needed to avoid load errors for modules (subagents, drivers, etc.)
	# Add it here to avoid breaking configure tests
	# Preserve original libs for libjansson build
	JANSSON_LIBS="$LIBS"
	LIBS="$LIBS -lCrunG3"
fi

AM_CONDITIONAL([ALL_STATIC], [test "x$ALL_STATIC" = "xyes"])
AM_CONDITIONAL([JAR_BUILD], [test "x$JAR_BUILD" = "xyes"])
AM_CONDITIONAL([MQTT_SUPPORT], [test "x$MQTT_SUPPORT" = "xyes"])
AM_CONDITIONAL([PYTHON_SUPPORT], [test "x$PYTHON_SUPPORT" = "xyes"])
AM_CONDITIONAL([RELEASE_BUILD], [test "x$RELEASE_BUILD" = "xyes"])
AM_CONDITIONAL([STATIC_BUILD], [test "x$STATIC_BUILD" = "xyes"])
AM_CONDITIONAL([USE_INTERNAL_JANSSON], [test "x$HAVE_JANSSON" = "xno"])
AM_CONDITIONAL([USE_INTERNAL_SQLITE], [test "x$HAVE_SQLITE" = "xno"])
AM_CONDITIONAL([USE_ENCRYPTION], [test "x${WITH_ENCRYPTION}" = "xyes"])
AM_CONDITIONAL([USE_LIBJQ], [test "x$HAVE_LIBJQ" = "xyes"])
AM_CONDITIONAL([WITH_PRIVATE_EXTENSIONS], [test "x$WITH_PRIVATE_EXTENSIONS" = "xyes"])


#--------------------------------------------------------------------
# Get compiler version
#--------------------------------------------------------------------

COMPILER_VERSION=""
if test "$CXX" = "CC" -o "$CXX" = "CC -std=c++11" ; then
	v=`$CXX -V 2>&1 | head -1`
	if test $? = 0; then
		COMPILER_VERSION="$v"
	fi
else
	for opt in -qversion --version -v -V; do
		v=`$CXX $opt 2>/dev/null | head -1`
		if test $? = 0; then
			if test "x$v" != "x" -a "x$COMPILER_VERSION" = "x"; then
				COMPILER_VERSION="$v"
			fi
		fi
	done
fi
AC_DEFINE_UNQUOTED([CPP_COMPILER_VERSION], "$COMPILER_VERSION", [C++ compiler version information])


#--------------------------------------------------------------------
# Cleanup variables
#--------------------------------------------------------------------

AGENT_DIRS=`echo $AGENT_DIRS|perl -nae 'my %seen; my @out; for (@F) {push(@out, $_) unless $seen{$_}++; }; print join(" ", @out)'`
HDLINK_DIRS=`echo $HDLINK_DIRS|perl -nae 'my %seen; my @out; for (@F) {push(@out, $_) unless $seen{$_}++; }; print join(" ", @out)'`
MODULES=`echo $MODULES|perl -nae 'my %seen; my @out; for (@F) {push(@out, $_) unless $seen{$_}++; }; print join(" ", @out)'`
NCDRV_MODULES=`echo $NCDRV_MODULES|perl -nae 'my %seen; my @out; for (@F) {push(@out, $_) unless $seen{$_}++; }; print join(" ", @out)'`
SUBAGENT_DIRS=`echo $SUBAGENT_DIRS|perl -nae 'my %seen; my @out; for (@F) {push(@out, $_) unless $seen{$_}++; }; print join(" ", @out)'`

NCDRV_LTLIBRARIES=`echo $NCDRV_MODULES|perl -nae 'my @out; for (@F) {push(@out, $_ . ".la"); }; print join(" ", @out)'`
DB_DRIVERS_LTLIBRARIES=`echo $DB_DRIVERS|perl -nae 'my @out; for (@F) {push(@out, $_ . ".la"); }; print join(" ", @out)'`

#--------------------------------------------------------------------
# Substitute variables
#--------------------------------------------------------------------

AC_SUBST(ABSEIL_CPPFLAGS)
AC_SUBST(ABSEIL_LDFLAGS)
AC_SUBST(AGENT_DIRS)
AC_SUBST(AGENT_LIBS)
AC_SUBST(AGENT_UNIT_TESTS)
AC_SUBST(ASTERISK_LIBS)
AC_SUBST(BUILD_SERVER)
AC_SUBST(BUILD_SDK)
AC_SUBST(CLIENT_COMPONENTS)
AC_SUBST(CONTRIB_MODULES)
AC_SUBST(CUSTOM_INCLUDE)
AC_SUBST(DB_DRIVERS)
AC_SUBST(DB_DRIVERS_LTLIBRARIES)
AC_SUBST(DB2_CPPFLAGS)
AC_SUBST(DB2_LDFLAGS)
AC_SUBST(DB2_LIBS)
AC_SUBST(EXEC_LDFLAGS)
AC_SUBST(EXEC_LIBS)
AC_SUBST(HDLINK_DIRS)
AC_SUBST(INFORMIX_CPPFLAGS)
AC_SUBST(INFORMIX_LDFLAGS)
AC_SUBST(INFORMIX_LIBS)
AC_SUBST(INSTALL_LIBPATH)
AC_SUBST(JANSSON_LIBS)
AC_SUBST(LIBISOTREE_CPPFLAGS)
AC_SUBST(LIBISOTREE_LDFLAGS)
AC_SUBST(LIBNXAGENT_LIBS)
AC_SUBST(LIBNXSRV_LIBS)
AC_SUBST(LIBSSH_CPPFLAGS)
AC_SUBST(LIBSSH_LDFLAGS)
AC_SUBST(LIBMODBUS_CPPFLAGS)
AC_SUBST(LIBMODBUS_LDFLAGS)
AC_SUBST(LIBJQ_CPPFLAGS)
AC_SUBST(LIBJQ_LDFLAGS)
AC_SUBST(MARIADB_CPPFLAGS)
AC_SUBST(MARIADB_LDFLAGS)
AC_SUBST(MARIADB_LIBS)
AC_SUBST(MICROHTTPD_CPPFLAGS)
AC_SUBST(MICROHTTPD_LDFLAGS)
AC_SUBST(MICROHTTPD_LIBS)
AC_SUBST(MODULES)
AC_SUBST(MONGODB_CPPFLAGS)
AC_SUBST(MONGODB_LDFLAGS)
AC_SUBST(MONGODB_LIBS)
AC_SUBST(MQTT_CPPFLAGS)
AC_SUBST(MQTT_LDFLAGS)
AC_SUBST(MQTT_LIBS)
AC_SUBST(MYSQL_CPPFLAGS)
AC_SUBST(MYSQL_LDFLAGS)
AC_SUBST(MYSQL_LIBS)
AC_SUBST(NCDRV_LTLIBRARIES)
AC_SUBST(NCDRV_MODULES)
AC_SUBST(NETXMS_BINDIR)
AC_SUBST(NETXMS_DATADIR)
AC_SUBST(NETXMS_JAR_VERSION)
AC_SUBST(NETXMS_LIBDIR)
AC_SUBST(NETXMS_PKGLIBDIR)
AC_SUBST(NETXMS_PREFIX)
AC_SUBST(NETXMS_STATEDIR)
AC_SUBST(NETXMS_SYSCONFDIR)
AC_SUBST(NXADM_LIBS)
AC_SUBST(OBJECT_MODE)
AC_SUBST(ODBC_CPPFLAGS)
AC_SUBST(ODBC_LDFLAGS)
AC_SUBST(ODBC_LIBS)
AC_SUBST(ORACLE_CPPFLAGS)
AC_SUBST(ORACLE_LDFLAGS)
AC_SUBST(ORACLE_LIBS)
AC_SUBST(PDSDRV_DIRS)
AC_SUBST(PGSQL_CPPFLAGS)
AC_SUBST(PGSQL_LDFLAGS)
AC_SUBST(PGSQL_LIBS)
AC_SUBST(PROMETHEUS_LIBS)
AC_SUBST(PROTOBUF_CPPFLAGS)
AC_SUBST(PROTOBUF_LDFLAGS)
AC_SUBST(PROTOBUF_LIBS)
AC_SUBST(PYTHON_CPPFLAGS)
AC_SUBST(PYTHON_LDFLAGS)
AC_SUBST(PYTHON_LIBS)
AC_SUBST(SNAPPY_CPPFLAGS)
AC_SUBST(SNAPPY_LDFLAGS)
AC_SUBST(SNAPPY_LIBS)
AC_SUBST(SERVER_COMPONENTS)
AC_SUBST(SERVER_INCLUDE)
AC_SUBST(SERVER_LIBS)
AC_SUBST(SERVER_TOOLS)
AC_SUBST(SHLIB_SUFFIX)
AC_SUBST(SQLFLAGS)
AC_SUBST(STATIC_SUBAGENT_LIST)
AC_SUBST(SUBAGENT_DIRS)
AC_SUBST(SUBAGENT_LIBS)
AC_SUBST(SYSTEMD_LIBS)
AC_SUBST(TEST_MODULES)
AC_SUBST(TOOLS)
AC_SUBST(TOP_LEVEL_MODULES)
AC_SUBST(TUXEDO_CPPFLAGS)
AC_SUBST(TUXEDO_LDFLAGS)
AC_SUBST(TUXEDO_LIBS)
AC_SUBST(TUXEDO_TPINIT)
AC_SUBST(VMGR_CPPFLAGS)
AC_SUBST(VMGR_LDFLAGS)
AC_SUBST(VMGR_LIBS)
AC_SUBST(XEN_CPPFLAGS)
AC_SUBST(XEN_LDFLAGS)
AC_SUBST(XEN_LIBS)
AC_SUBST(ZEROMQ_CPPFLAGS)
AC_SUBST(ZEROMQ_LDFLAGS)

NETXMS_LIBRARY_VERSION=`echo $VERSION | cut -f 1 -d '-' | cut -f 1,2 -d . | sed 's/\.//'`
AC_SUBST(NETXMS_LIBRARY_VERSION)

#--------------------------------------------------------------------
# Generate output files
#--------------------------------------------------------------------

AC_CONFIG_FILES([
	Makefile
	build/Makefile
	contrib/Makefile
	contrib/backgrounds/Makefile
	contrib/integration/Makefile
	contrib/integration/HP_EVA/Makefile
	contrib/integration/HP_P2000/Makefile
	contrib/mibs/Makefile
	contrib/music/Makefile
	contrib/oui/Makefile
	contrib/startup/Makefile
	contrib/startup/systemd/Makefile
	contrib/startup/debian/Makefile
	contrib/startup/gentoo/Makefile
	contrib/startup/hp-ux/Makefile
	contrib/startup/redhat/Makefile
	contrib/startup/suse/Makefile
	contrib/startup/ubuntu/Makefile
	contrib/startup/solaris/Makefile
	contrib/templates/Makefile
	doc/Makefile
	doc/internal/Makefile
	images/Makefile
	include/Makefile
	include/jansson/Makefile
	include/msvc/Makefile
	m4/Makefile
	manpages/Makefile
	src/Makefile
	src/agent/Makefile
	src/agent/core/Makefile
	src/agent/install/Makefile
	src/agent/libnxagent/Makefile
	src/agent/libnxappc/Makefile
	src/agent/libnxsde/Makefile
	src/agent/libnxtux/Makefile
	src/agent/smbios/Makefile
	src/agent/subagents/Makefile
	src/agent/subagents/aix/Makefile
	src/agent/subagents/asterisk/Makefile
	src/agent/subagents/bind9/Makefile
	src/agent/subagents/darwin/Makefile
	src/agent/subagents/db2/Makefile
	src/agent/subagents/dbquery/Makefile
	src/agent/subagents/devemu/Makefile
	src/agent/subagents/ds18x20/Makefile
	src/agent/subagents/filemgr/Makefile
	src/agent/subagents/freebsd/Makefile
	src/agent/subagents/gps/Makefile
	src/agent/subagents/informix/Makefile
	src/agent/subagents/java/Makefile
	src/agent/subagents/java/java/Makefile
	src/agent/subagents/jmx/Makefile
	src/agent/subagents/linux/Makefile
	src/agent/subagents/lmsensors/Makefile
	src/agent/subagents/logwatch/Makefile
	src/agent/subagents/minix/Makefile
	src/agent/subagents/mqtt/Makefile
	src/agent/subagents/mysql/Makefile
	src/agent/subagents/netbsd/Makefile
	src/agent/subagents/netsvc/Makefile
	src/agent/subagents/opcua/Makefile
	src/agent/subagents/openbsd/Makefile
	src/agent/subagents/oracle/Makefile
	src/agent/subagents/pgsql/Makefile
	src/agent/subagents/ping/Makefile
	src/agent/subagents/python/Makefile
	src/agent/subagents/prometheus/Makefile
	src/agent/subagents/redis/Makefile
	src/agent/subagents/rpi/Makefile
	src/agent/subagents/sms/Makefile
	src/agent/subagents/ssh/Makefile
	src/agent/subagents/sunos/Makefile
	src/agent/subagents/tuxedo/Makefile
	src/agent/subagents/mongodb/Makefile
	src/agent/subagents/ubntlw/Makefile
	src/agent/subagents/ups/Makefile
	src/agent/subagents/vmgr/Makefile
	src/agent/subagents/winnt/Makefile
	src/agent/subagents/winperf/Makefile
	src/agent/subagents/wmi/Makefile
	src/agent/subagents/xen/Makefile
	src/agent/tools/Makefile
	src/agent/tools/nxaevent/Makefile
	src/agent/tools/nxappget/Makefile
	src/agent/tools/nxapush/Makefile
	src/agent/tools/nxhwid/Makefile
	src/agent/tools/nxreload/Makefile
	src/agent/tools/nxtftp/Makefile
	src/agent/tools/scripts/Makefile
	src/appagent/Makefile
	src/client/Makefile
	src/client/java/Makefile
	src/client/java/netxms-client/Makefile
	src/client/libnxclient/Makefile
	src/client/nxalarm/Makefile
	src/client/nxapisrv/Makefile
	src/client/nxapisrv/java/Makefile
	src/client/nxevent/Makefile
	src/client/nxmc/Makefile
	src/client/nxmc/java/Makefile
	src/client/nxnotify/Makefile
	src/client/nxpush/Makefile
	src/client/nxshell/Makefile
	src/client/nxshell/java/Makefile
	src/db/Makefile
	src/db/dbdrv/Makefile
	src/db/dbdrv/db2/Makefile
	src/db/dbdrv/informix/Makefile
	src/db/dbdrv/odbc/Makefile
	src/db/dbdrv/mariadb/Makefile
	src/db/dbdrv/mssql/Makefile
	src/db/dbdrv/mysql/Makefile
	src/db/dbdrv/oracle/Makefile
	src/db/dbdrv/pgsql/Makefile
	src/db/dbdrv/sqlite/Makefile
	src/db/libnxdb/Makefile
	src/ethernetip/Makefile
	src/ethernetip/libethernetip/Makefile
	src/ethernetip/nxethernetip/Makefile
	src/flow-collector/Makefile
	src/flow-collector/libipfix/Makefile
	src/install/Makefile
	src/install/windows/Makefile
	src/jansson/Makefile
	src/java-common/Makefile
	src/java-common/netxms-base/Makefile
	src/libnetxms/Makefile
	src/libnxcc/Makefile
	src/libnxjava/Makefile
	src/libnxjava/java/Makefile
	src/libnxlp/Makefile
	src/libnxmb/Makefile
	src/libnxpython/Makefile
	src/libnxsl/Makefile
	src/mobile-agent/Makefile
	src/mobile-agent/java/Makefile
	src/ncdrivers/Makefile
	src/ncdrivers/anysms/Makefile
	src/ncdrivers/dbtable/Makefile
	src/ncdrivers/dummy/Makefile
	src/ncdrivers/gsm/Makefile
	src/ncdrivers/googlechat/Makefile
	src/ncdrivers/kannel/Makefile
	src/ncdrivers/mattermost/Makefile
	src/ncdrivers/mqtt/Makefile
	src/ncdrivers/msteams/Makefile
	src/ncdrivers/mymobile/Makefile
	src/ncdrivers/nexmo/Makefile
	src/ncdrivers/nxagent/Makefile
	src/ncdrivers/portech/Makefile
	src/ncdrivers/shell/Makefile
	src/ncdrivers/slack/Makefile
	src/ncdrivers/smseagle/Makefile
	src/ncdrivers/smtp/Makefile
	src/ncdrivers/snmptrap/Makefile
	src/ncdrivers/telegram/Makefile
	src/ncdrivers/text2reach/Makefile
	src/ncdrivers/textfile/Makefile
	src/ncdrivers/twilio/Makefile
	src/ncdrivers/websms/Makefile
	src/ncdrivers/xmpp/Makefile
	src/nxcproxy/Makefile
	src/nxscript/Makefile
	src/server/Makefile
	src/server/core/Makefile
	src/server/drivers/Makefile
	src/server/drivers/at/Makefile
	src/server/drivers/avaya/Makefile
	src/server/drivers/cambium/Makefile
	src/server/drivers/cisco/Makefile
	src/server/drivers/dell-pwc/Makefile
	src/server/drivers/dlink/Makefile
	src/server/drivers/edgecore/Makefile
	src/server/drivers/eltex/Makefile
	src/server/drivers/etherwan/Makefile
	src/server/drivers/extreme/Makefile
	src/server/drivers/fortinet/Makefile
	src/server/drivers/hirschmann/Makefile
	src/server/drivers/hpe/Makefile
	src/server/drivers/huawei/Makefile
	src/server/drivers/ignitenet/Makefile
	src/server/drivers/juniper/Makefile
	src/server/drivers/mds/Makefile
	src/server/drivers/mikrotik/Makefile
	src/server/drivers/moxa/Makefile
	src/server/drivers/net-snmp/Makefile
	src/server/drivers/netonix/Makefile
	src/server/drivers/ping3/Makefile
	src/server/drivers/planet/Makefile
	src/server/drivers/qtech/Makefile
	src/server/drivers/rittal/Makefile
	src/server/drivers/saf/Makefile
	src/server/drivers/siemens/Makefile
	src/server/drivers/symbol-ws/Makefile
	src/server/drivers/tb/Makefile
	src/server/drivers/tplink/Makefile
	src/server/drivers/ubnt/Makefile
	src/server/drivers/westerstrand/Makefile
	src/server/hdlink/Makefile
	src/server/hdlink/jira/Makefile
	src/server/hdlink/redmine/Makefile
	src/server/include/Makefile
	src/server/leef/Makefile
	src/server/libnxsrv/Makefile
	src/server/netxmsd/Makefile
	src/server/ntcb/Makefile
	src/server/nxreportd/Makefile
	src/server/nxreportd/java/Makefile
	src/server/pdsdrv/Makefile
	src/server/pdsdrv/clickhouse/Makefile
	src/server/pdsdrv/influxdb/Makefile
	src/server/pdsdrv/rrdtool/Makefile
	src/server/spe/Makefile
	src/server/tools/Makefile
	src/server/tools/libnxdbmgr/Makefile
	src/server/tools/nddload/Makefile
	src/server/tools/nxaction/Makefile
	src/server/tools/nxadm/Makefile
	src/server/tools/nxap/Makefile
	src/server/tools/nxdbmgr/Makefile
	src/server/tools/nxdownload/Makefile
	src/server/tools/nxget/Makefile
	src/server/tools/nxminfo/Makefile
	src/server/tools/nxupload/Makefile
	src/server/tools/nxwsget/Makefile
	src/server/tools/scripts/Makefile
	src/server/webapi/Makefile
	src/server/wcc/Makefile
	src/server/wlcbridge/Makefile
	src/snmp/Makefile
	src/snmp/libnxsnmp/Makefile
	src/snmp/nxmibc/Makefile
	src/snmp/nxsnmpget/Makefile
	src/snmp/nxsnmpset/Makefile
	src/snmp/nxsnmpwalk/Makefile
	src/sqlite/Makefile
	src/tools/Makefile
	src/tools/nxcrashsrv/Makefile
	src/tools/nxcsum/Makefile
	src/tools/nxdevcfg/Makefile
	src/tools/nxencpasswd/Makefile
	src/tools/nxgenguid/Makefile
	src/tools/nxlptest/Makefile
	src/tools/nxping/Makefile
	src/tools/scripts/Makefile
	sql/Makefile
	tests/Makefile
	tests/agent/Makefile
	tests/agent/unit/Makefile
	tests/agent/unit/linux-cpu-usage-collector/Makefile
	tests/config/Makefile
	tests/include/Makefile
	tests/suite/Makefile
	tests/test-libnetxms/Makefile
	tests/test-libnxcc/Makefile
	tests/test-libnxdb/Makefile
	tests/test-libnxsl/Makefile
	tests/test-libnxsnmp/Makefile
	tools/Makefile
])

AC_OUTPUT

echo "Updating libtool"
sed 's,$SED -e "/${host}-//g",$SED -e "s/${host}-//g",g' < ./libtool > libtool.new && mv libtool.new libtool && chmod +x ./libtool

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
echo "Version                 : ${VERSION}"
echo "Prefix                  : ${prefix}"
echo "Platform                : ${PLATFORM}"
echo "Shared library suffix   : ${SHLIB_SUFFIX}"
echo "Shared library version  : ${NETXMS_LIBRARY_VERSION}"
if test "x${WITH_ENCRYPTION}" = "xyes"; then
	echo "Encryption enabled      : YES"
else
	echo "Encryption enabled      : NO"
fi
if test "x$IPV6_SUPPORT" = "xyes"; then
	echo "IPv6 support            : YES"
else
	echo "IPv6 support            : NO"
fi
if test "x${JDK_LOCATION}" != "x"; then
	echo "JDK location            : ${JDK_LOCATION}"
fi

if test "x${BUILD_SERVER}" = "xyes"; then
	echo "Build Server            : YES"
	echo "Server libs             : ${SERVER_LIBS}"
	if test "x${LDAP_SUPPORT}" = "xyes"; then
		echo "LDAP support            : YES"
	else
		echo "LDAP support            : NO"
	fi
	if test "x${MICROHTTPD_SUPPORT}" = "xyes"; then
		echo "MicroHTTPD support      : YES"
	else
		echo "MicroHTTPD support      : NO"
	fi
	if test "x${HAVE_LIBEDIT}" = "xyes"; then
		echo "Editline support        : YES"
	else
		echo "Editline support        : NO"
	fi
	if test "x${HAVE_LIBISOTREE}" = "xyes"; then
		echo "Isolation tree support  : YES"
	else
		echo "Isolation tree support  : NO"
	fi
	echo "Server components       : ${SERVER_COMPONENTS}"
	echo "Fan-out data drivers    : ${PDSDRV_DIRS}"
	echo "Helpdesk system drivers : ${HDLINK_DIRS}"
else
	echo "Build Server            : NO"
fi

if test "x${XMPP_SUPPORT}" = "xyes"; then
   echo "XMPP support            : YES"
else
   echo "XMPP support            : NO"
fi

if test "x${PYTHON_SUPPORT}" = "xyes"; then
	echo "Python support          : YES"
	echo "Python version          : $PYTHON_VERSION"
else
	echo "Python support          : NO"
fi
if test "x${PYTHON_CPPFLAGS}" != "x"; then
	echo "Python CPPFLAGS         : ${PYTHON_CPPFLAGS}"
fi
if test "x${PYTHON_LDFLAGS}" != "x"; then
	echo "Python LDFLAGS          : ${PYTHON_LDFLAGS}"
fi
if test "x${PYTHON_LIBS}" != "x"; then
	echo "Python LIBS             : ${PYTHON_LIBS}"
fi

if test "x${HAVE_LIBJQ}" = "xyes"; then
	echo "JQ support              : YES"
else
	echo "JQ support              : NO"
fi
if test "x${HAVE_LIBSSH}" = "xyes"; then
	echo "SSH support             : YES"
else
	echo "SSH support             : NO"
fi
if test "x${MQTT_SUPPORT}" = "xyes"; then
	echo "MQTT support            : YES"
else
	echo "MQTT support            : NO"
fi
if test "x${HAVE_MODBUS}" = "xyes"; then
	echo "MODBUS support          : YES"
else
	echo "MODBUS support          : NO"
fi
if test "x${ZEROMQ_SUPPORT}" = "xyes"; then
	echo "ZeroMQ support          : YES"
   if test "x${ZEROMQ_CPPFLAGS}" != "x"; then
      echo "ZeroMQ CPPFLAGS         : ${ZEROMQ_CPPFLAGS}"
   fi
   if test "x${ZEROMQ_LDFLAGS}" != "x"; then
      echo "ZeroMQ LDFLAGS          : ${ZEROMQ_LDFLAGS}"
   fi
else
	echo "ZeroMQ support          : NO"
fi
if test "x${DB_DRIVERS}" != "x"; then
	echo "Build DB-Drivers        :${DB_DRIVERS}"
else
	echo "Build DB-Drivers        : NO"
fi
if test "x${BUILD_CLIENT}" = "xyes"; then
	echo "Build Clients           : YES"
	echo "Client components       :${CLIENT_COMPONENTS}"
else
	echo "Build Clients           : NO"
fi
if test "x${BUILD_AGENT}" = "xyes"; then
	echo "Build Agent             : YES"
	echo "Agent libs              :${AGENT_LIBS}"
	echo "Agent extra components  : ${AGENT_DIRS}"
	echo "Subagent list           : ${SUBAGENT_DIRS}"
	if test "x${SYSTEMD_SUPPORT}" = "xyes"; then
		echo "Systemd support         : YES"
		if test "x${HAVE_SDBUS}" = "xyes"; then
			echo "SD-Bus support          : YES"
		else
			echo "SD-Bus support          : NO"
		fi
	else
		echo "Systemd support         : NO"
	fi
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
if test "x${HAVE_JANSSON}" = "xno"; then
	echo "Use internal libjansson : YES"
else
	echo "Use internal libjansson : NO"
fi
check_substr "$COMPONENTS" "sqlite"
if test $? = 0; then
	if test "x${HAVE_SQLITE}" = "xno"; then
		echo "Use internal sqlite     : YES"
	else
		echo "Use internal sqlite     : NO"
	fi
fi
if test "x${FORCE_32BIT_BUILD}" = "xyes"; then
	echo "Force 32bit build       : YES"
else
	echo "Force 32bit build       : NO"
fi

echo "C compiler              : ${CC}"
echo "C++ compiler            : ${CXX}"
echo "C++ compiler info       : ${COMPILER_VERSION}"
echo "CPPFLAGS                : ${CPPFLAGS}"
echo "CXXFLAGS                : ${CXXFLAGS}"
echo "CFLAGS                  : ${CFLAGS}"
echo "LDFLAGS                 : ${LDFLAGS}"
echo "LIBS                    : ${LIBS}"
echo "Executable LDFLAGS      : ${EXEC_LDFLAGS}"
echo "Executable LIBS         : ${EXEC_LIBS}"

if test "x${MYSQL_CPPFLAGS}" != "x"; then
	echo "MySQL CPPFLAGS          : ${MYSQL_CPPFLAGS}"
fi
if test "x${MYSQL_LDFLAGS}" != "x"; then
	echo "MySQL LDFLAGS           : ${MYSQL_LDFLAGS}"
fi
if test "x${MYSQL_LIBS}" != "x"; then
	echo "MySQL LIBS              : ${MYSQL_LIBS}"
fi

if test "x${MARIADB_CPPFLAGS}" != "x"; then
	echo "MariaDB CPPFLAGS        : ${MARIADB_CPPFLAGS}"
fi
if test "x${MARIADB_LDFLAGS}" != "x"; then
	echo "MariaDB LDFLAGS         : ${MARIADB_LDFLAGS}"
fi
if test "x${MARIADB_LIBS}" != "x"; then
	echo "MariaDB LIBS            : ${MARIADB_LIBS}"
fi

if test "x${PGSQL_CPPFLAGS}" != "x"; then
	echo "PostgreSQL CPPFLAGS     : ${PGSQL_CPPFLAGS}"
fi
if test "x${PGSQL_LDFLAGS}" != "x"; then
	echo "PostgreSQL LDFLAGS      : ${PGSQL_LDFLAGS}"
fi
if test "x${PGSQL_LIBS}" != "x"; then
	echo "PostgreSQL LIBS         : ${PGSQL_LIBS}"
fi

if test "x${ORACLE_CPPFLAGS}" != "x"; then
	echo "Oracle CPPFLAGS         : ${ORACLE_CPPFLAGS}"
fi
if test "x${ORACLE_LDFLAGS}" != "x"; then
	echo "Oracle LDFLAGS          : ${ORACLE_LDFLAGS}"
fi
if test "x${ORACLE_LIBS}" != "x"; then
	echo "Oracle LIBS             : ${ORACLE_LIBS}"
fi

if test "x${ODBC_CPPFLAGS}" != "x"; then
	echo "ODBC CPPFLAGS           : ${ODBC_CPPFLAGS}"
fi
if test "x${ODBC_LDFLAGS}" != "x"; then
	echo "ODBC LDFLAGS            : ${ODBC_LDFLAGS}"
fi
if test "x${ODBC_LIBS}" != "x"; then
	echo "ODBC LIBS               : ${ODBC_LIBS}"
fi

if test "x${DB2_CPPFLAGS}" != "x"; then
	echo "DB2 CPPFLAGS            : ${DB2_CPPFLAGS}"
fi
if test "x${DB2_LDFLAGS}" != "x"; then
	echo "DB2 LDFLAGS             : ${DB2_LDFLAGS}"
fi
if test "x${DB2_LIBS}" != "x"; then
	echo "DB2 LIBS                : ${DB2_LIBS}"
fi

if test "x${INFORMIX_CPPFLAGS}" != "x"; then
	echo "Informix CPPFLAGS       : ${INFORMIX_CPPFLAGS}"
fi
if test "x${INFORMIX_LDFLAGS}" != "x"; then
	echo "Informix LDFLAGS        : ${INFORMIX_LDFLAGS}"
fi
if test "x${INFORMIX_LIBS}" != "x"; then
	echo "Informix LIBS           : ${INFORMIX_LIBS}"
fi

if test "x${TUXEDO_CPPFLAGS}" != "x"; then
	echo "Tuxedo CPPFLAGS         : ${TUXEDO_CPPFLAGS}"
fi
if test "x${TUXEDO_LDFLAGS}" != "x"; then
	echo "Tuxedo LDFLAGS          : ${TUXEDO_LDFLAGS}"
fi
if test "x${TUXEDO_LIBS}" != "x"; then
	echo "Tuxedo LIBS             : ${TUXEDO_LIBS}"
fi

if test "x${LIBMODBUS_CPPFLAGS}" != "x"; then
	echo "libmodbus CPPFLAGS      : ${LIBMODBUS_CPPFLAGS}"
fi
if test "x${LIBMODBUS_LDFLAGS}" != "x"; then
	echo "libmodbus LDFLAGS       : ${LIBMODBUS_LDFLAGS}"
fi

if test "x${MONGODB_CPPFLAGS}" != "x"; then
	echo "MongoDB CPPFLAGS        : ${MONGODB_CPPFLAGS}"
fi
if test "x${MONGODB_LDFLAGS}" != "x"; then
	echo "MongoDB LDFLAGS         : ${MONGODB_LDFLAGS}"
fi

if test "x${ASTERISK_LIBS}" != "x"; then
	echo "Asterisk LIBS           :${ASTERISK_LIBS}"
fi

if test "x${VMGR_CPPFLAGS}" != "x"; then
	echo "Vmgr CPPFLAGS           : ${VMGR_CPPFLAGS}"
fi
if test "x${VMGR_LDFLAGS}" != "x"; then
	echo "Vmgr LDFLAGS            : ${VMGR_LDFLAGS}"
fi
if test "x${VMGR_LIBS}" != "x"; then
	echo "Vmgr LIBS               : ${VMGR_LIBS}"
fi

if test "x${XEN_CPPFLAGS}" != "x"; then
	echo "XEN CPPFLAGS            : ${XEN_CPPFLAGS}"
fi
if test "x${XEN_LDFLAGS}" != "x"; then
	echo "XEN LDFLAGS             : ${XEN_LDFLAGS}"
fi
if test "x${XEN_LIBS}" != "x"; then
	echo "XEN LIBS                : ${XEN_LIBS}"
fi

FLAGS_CPP="src/tools/nxdevcfg/flags.cpp"
echo "/* Automatically generated by configure */" > $FLAGS_CPP
echo "#include <nms_common.h>" >> $FLAGS_CPP
echo "const TCHAR *g_cppFlags = _T(\"${CPPFLAGS}\");" >> $FLAGS_CPP
echo "const TCHAR *g_cFlags = _T(\"${CFLAGS}\");" >> $FLAGS_CPP
echo "const TCHAR *g_cxxFlags = _T(\"${CXXFLAGS}\");" >> $FLAGS_CPP
echo "const TCHAR *g_ldFlags = _T(\"${LDFLAGS}\");" >> $FLAGS_CPP
echo "const TCHAR *g_execLdFlags = _T(\"${EXEC_LDFLAGS}\");" >> $FLAGS_CPP
echo "const TCHAR *g_libs = _T(\"${LIBS}\");" >> $FLAGS_CPP
echo "const TCHAR *g_execLibs = _T(\"${EXEC_LIBS}\");" >> $FLAGS_CPP
echo "const TCHAR *g_cc = _T(\"${CC}\");" >> $FLAGS_CPP
echo "const TCHAR *g_cxx = _T(\"${CXX}\");" >> $FLAGS_CPP
echo "const TCHAR *g_ld = _T(\"${LD}\");" >> $FLAGS_CPP
echo "const TCHAR *g_perl = _T(\"${PERL}\");" >> $FLAGS_CPP
echo "const TCHAR *g_serverLibs = _T(\"${SERVER_LIBS}\");" >> $FLAGS_CPP
echo "const TCHAR *g_tuxedoCppFlags = _T(\"${TUXEDO_CPPFLAGS}\");" >> $FLAGS_CPP
echo "const TCHAR *g_tuxedoLdFlags = _T(\"${TUXEDO_LDFLAGS}\");" >> $FLAGS_CPP
echo "const TCHAR *g_tuxedoLibs = _T(\"${TUXEDO_LIBS}\");" >> $FLAGS_CPP
