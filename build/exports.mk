#
# exports.mk - variable export list shared by the top-level Makefile.w32 and
# any private-subsystem dispatcher (e.g. private/atm/Makefile.w32) that can be
# invoked standalone. Include this AFTER config.mingw (and after any
# normalization/tier forcing) so the exported values are final.
#
# Sub-makes inherit these via the environment; component Makefiles rely on them
# instead of re-including config.mingw.
#

# Parser generators (flex/bison). These are host tools used to regenerate the
# lexer/parser sources for libnxsl and nxmibc, so they are not cross-prefixed.
# Defaulted here (rather than only in config.mingw.template) so existing
# config.mingw files that predate these variables still build.
LEX ?= flex
YACC ?= bison

export TOPDIR CC CXX AR RANLIB WINDRES STRIP LD LEX YACC
export CONFIG WINXP PLATFORM TOOLCHAIN_BIN
export ARCH DEBUG PREFIX
export BUILD_AGENT BUILD_CLIENT BUILD_SERVER
export COMMON_CFLAGS COMMON_CXXFLAGS COMMON_LDFLAGS COMMON_LIBDIRS
export WIN_LIBS SSL_LIBS PCRE_LIBS CURL_LIBS ZLIB_LIBS EXPAT_LIBS JQ_LIBS MODBUS_LIBS LIBSSH_LIBS
export MOSQUITTO_LIBS PROTOBUF_LIBS MICROHTTPD_LIBS SNAPPY_LIBS STROPHE_LIBS
export PGSQL_LIBS MYSQL_LIBS MARIADB_LIBS ORACLE_LIBS DB2_LIBS INFORMIX_LIBS
export BUILD_DIR BIN_DIR LIB_DIR OBJ_DIR
export VERBOSE Q
export OPENSSL_ROOT PCRE_ROOT ZLIB_ROOT CURL_ROOT EXPAT_ROOT JQ_ROOT MODBUS_ROOT LIBSSH_ROOT
export MOSQUITTO_ROOT JNI_ROOT PROTOBUF_ROOT MICROHTTPD_ROOT SNAPPY_ROOT LIBSTROPHE_ROOT
export PGSQL_ROOT MYSQL_ROOT MARIADB_ROOT ORACLE_ROOT DB2_ROOT INFORMIX_ROOT

# ATM subsystem SDKs (private/atm)
export XFS_ROOT IRISGUARD_ROOT DSVCAP_ROOT OPENH264_ROOT MP4V2_ROOT
export XFS_LIBS XFS_CPPFLAGS IRISGUARD_LIBS IRISGUARD_CPPFLAGS
export DSVCAP_LIBS DSVCAP_CPPFLAGS OPENH264_LIBS OPENH264_CPPFLAGS MP4V2_LIBS MP4V2_CPPFLAGS
