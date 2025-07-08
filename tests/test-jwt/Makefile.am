# Copyright (C) 2024 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = test-jwt
test_jwt_SOURCES = test-jwt.cpp
test_jwt_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I../include -I@top_srcdir@/build
test_jwt_LDFLAGS = @EXEC_LDFLAGS@
test_jwt_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/server/core/libnxcore.la @EXEC_LIBS@