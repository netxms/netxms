lib_LTLIBRARIES = libnetxms.la

libnetxms_la_SOURCES = \
	array.cpp base32.cpp base64.cpp bytestream.cpp calltbl.cpp cc_mb.cpp \
	cc_ucs2.cpp cc_ucs4.cpp cc_utf8.cpp cch.cpp cert.cpp config.cpp crypto.cpp \
	curl.cpp debug_tag_tree.cpp diff.cpp dirw_unix.cpp geolocation.cpp getopt.cpp \
	getoptw.cpp dload.cpp hash.cpp hashmapbase.cpp hashsetbase.cpp ice.c \
	icmp.cpp iconv.cpp inet_pton.cpp inetaddr.cpp itoa.cpp log.cpp lz4.c \
	macaddr.cpp md4.cpp md5.cpp memmem.cpp mempool.cpp \
	message.cpp msgrecv.cpp msgwq.cpp net.cpp nxcp.cpp npipe.cpp npipe_unix.cpp \
	pa.cpp procexec.cpp pugixml.cpp qsort.cpp queue.cpp rbuffer.cpp scandir.cpp serial.cpp \
	sha1.cpp sha2.cpp socket_listener.cpp spoll.cpp squeue.cpp strcasestr.cpp \
	streamcomp.cpp string.cpp stringlist.cpp strlcat.cpp strlcpy.cpp strmap.cpp \
	strmapbase.cpp strptime.cpp strset.cpp strtoll.cpp strtoull.cpp \
	subproc.cpp table.cpp tasks.cpp tfw.cpp threads.cpp timegm.cpp tls_conn.cpp \
	tools.cpp tp.cpp unicode.cpp uuid.cpp vault.cpp wcstoll.cpp wcstoull.cpp xml.cpp \
	wcscasecmp.cpp wcscasestr.cpp wcslcat.cpp wcslcpy.cpp wcsncasecmp.cpp ztools.cpp

libnetxms_la_CPPFLAGS=-I@top_srcdir@/include -DLIBNETXMS_EXPORTS -I@top_srcdir@/build
libnetxms_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnetxms_la_LIBADD =
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
	npipe_win32.cpp \
	windll.cpp
