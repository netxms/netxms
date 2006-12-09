INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnetxms.la
libnetxms_la_SOURCES = config.cpp crypto.cpp gen_uuid.c dload.cpp hash.cpp icmp.cpp inline.cpp main.cpp md5.cpp message.cpp msgwq.cpp nxcp.cpp queue.cpp rwlock.cpp scandir.c serial.cpp sha1.cpp strtoll.c strtoull.c table.cpp threads.cpp tools.cpp unicode.cpp uuid.c
libnetxms_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)

EXTRA_DIST = \
	libnetxms.dsp libnetxms.dsw \
	libnetxms.vcp libnetxms.vcw \
	Makefile.nw netxms.def \
	libnetxms.h md5.h sha1.h uuidP.h \
	regex.c c_lgcc3.cpp dir.c getopt.c
