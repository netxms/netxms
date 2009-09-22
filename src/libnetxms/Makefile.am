INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/libexpat/libexpat
SOURCES = agent.cpp base64.cpp config.cpp crypto.cpp gen_uuid.c \
	  geolocation.cpp dload.cpp hash.cpp icmp.cpp \
	  log.cpp main.cpp md5.cpp message.cpp msgwq.cpp \
	  nxcp.cpp qsort.c queue.cpp rwlock.cpp scandir.c serial.cpp \
	  sha1.cpp string.cpp stringlist.cpp strmap.cpp strtoll.c strtoull.c \
	  table.cpp threads.cpp tools.cpp unicode.cpp uuid.c wcstoll.c \
	  wcstoull.c xml.cpp

if BUILD_UNICODE_LIBS
lib_LTLIBRARIES = libnetxms.la libnetxmsw.la
else
lib_LTLIBRARIES = libnetxms.la
endif

libnetxms_la_SOURCES = $(SOURCES)
libnetxms_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
if USE_INTERNAL_EXPAT
libnetxms_la_LIBADD = ../libexpat/libexpat/libnxexpat.la
endif

libnetxmsw_la_SOURCES = $(SOURCES)
libnetxmsw_la_CPPFLAGS = -DUNICODE
libnetxmsw_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
if USE_INTERNAL_EXPAT
libnetxmsw_la_LIBADD = ../libexpat/libexpat/libnxexpat.la
endif

EXTRA_DIST = \
	libnetxms.vcproj libnetxmsw.vcproj \
	libnetxms.vcp libnetxms.vcw \
	Makefile.nw netxms.def \
	libnetxms.h md5.h sha1.h uuidP.h \
	regex.c c_lgcc3.cpp dir.c getopt.c \
	seh.cpp StackWalker.cpp StackWalker.h
