SOURCES = array.cpp base64.cpp bytestream.cpp cc_mb.cpp cc_ucs2.cpp \
          cc_ucs4.cpp cc_utf8.cpp cch.cpp config.cpp crypto.cpp debug_tag_tree.cpp diff.cpp \
	  dirw_unix.c geolocation.cpp getopt.c dload.cpp hash.cpp \
	  hashmapbase.cpp ice.c icmp.cpp icmp6.cpp iconv.cpp inet_pton.c \
	  inetaddr.cpp log.cpp lz4.c main.cpp md5.cpp message.cpp \
	  msgrecv.cpp msgwq.cpp net.cpp nxcp.cpp nxproc.cpp nxproc_unix.cpp \
	  pa.cpp parisc_atomic.cpp \
          qsort.c queue.cpp rbuffer.cpp rwlock.cpp scandir.c serial.cpp \
	  sha1.cpp sha2.cpp socket_listener.cpp solaris9_atomic.c spoll.cpp streamcomp.cpp \
	  string.cpp stringlist.cpp strlcat.c strlcpy.c strmap.cpp \
	  strmapbase.cpp strptime.c \
	  strset.cpp strtoll.c strtoull.c table.cpp threads.cpp timegm.c \
	  tools.cpp tp.cpp unicode.cpp uuid.cpp wcstoll.c wcstoull.c xml.cpp \
	  wcscasecmp.cpp wcslcat.c wcslcpy.c wcsncasecmp.cpp ztools.cpp

lib_LTLIBRARIES = libnetxms.la

if HPUX_HPPA
libnetxms_la_SOURCES = ldcw.s $(SOURCES)
else
libnetxms_la_SOURCES = $(SOURCES)
endif

libnetxms_la_CPPFLAGS=-I@top_srcdir@/include
if USE_INTERNAL_EXPAT
libnetxms_la_CPPFLAGS+=-I@top_srcdir@/src/libexpat/libexpat
endif
if USE_INTERNAL_ZLIB
libnetxms_la_CPPFLAGS += -I../zlib
endif

libnetxms_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)

libnetxms_la_LIBADD =
if USE_INTERNAL_EXPAT
libnetxms_la_LIBADD += ../libexpat/libexpat/libnxexpat.la
endif
if USE_INTERNAL_LIBTRE
libnetxms_la_LIBADD += ../libtre/libnxtre.la
endif
if USE_INTERNAL_ZLIB
libnetxms_la_LIBADD += ../zlib/libnxzlib.la
endif
if USE_INTERNAL_JANSSON
libnetxms_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
libnetxms_la_LIBADD += -ljansson
endif


EXTRA_DIST = \
	libnetxms.vcxproj libnetxms.vcxproj.filters \
	libnetxms.h diff.h ice.h lz4.h md5.h sha1.h sha2.h strmap-internal.h unicode_cc.h \
	debug_tag_tree.h \
	dir.cpp dirw.cpp \
	nxproc_win32.cpp \
	seh.cpp StackWalker.cpp StackWalker.h \
	ldcw.s
