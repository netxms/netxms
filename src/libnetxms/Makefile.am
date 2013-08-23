SOURCES = agent.cpp array.cpp base64.cpp config.cpp crypto.cpp dirw_unix.c \
	  gen_uuid.c geolocation.cpp getopt.c dload.cpp hash.cpp ice.c \
	  icmp.cpp log.cpp main.cpp md5.cpp message.cpp msgwq.cpp net.cpp \
	  nxcp.cpp qsort.c queue.cpp rwlock.cpp scandir.c serial.cpp \
	  sha1.cpp string.cpp stringlist.cpp strmap.cpp strmapbase.cpp \
	  strset.cpp strtoll.c strtoull.c table.cpp threads.cpp tools.cpp \
	  unicode.cpp uuid.c wcstoll.c wcstoull.c xml.cpp wcscasecmp.cpp

lib_LTLIBRARIES = libnetxms.la

libnetxms_la_SOURCES = $(SOURCES)
libnetxms_la_CPPFLAGS=-I@top_srcdir@/include
if USE_INTERNAL_EXPAT
libnetxms_la_CPPFLAGS+=-I@top_srcdir@/src/libexpat/libexpat
endif
libnetxms_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnetxms_la_LIBADD =
if USE_INTERNAL_EXPAT
libnetxms_la_LIBADD += ../libexpat/libexpat/libnxexpat.la
endif
if USE_INTERNAL_LIBTRE
libnetxms_la_LIBADD += ../libtre/libnxtre.la
endif

EXTRA_DIST = \
	libnetxms.vcproj \
	libnetxms.h ice.h md5.h sha1.h uuidP.h \
	dir.c dirw.c \
	seh.cpp StackWalker.cpp StackWalker.h
