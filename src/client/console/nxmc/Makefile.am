SUBDIRS = rc

INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/client/console/include

bin_PROGRAMS = nxmc
nxmc_SOURCES = \
	busydlg.cpp \
	comm.cpp \
	conlog.cpp \
	ctrlpanel.cpp \
	frame.cpp \
	logindlg.cpp \
	mainfrm.cpp \
	nxmc.cpp \
	plugins.cpp \
	srvcfg.cpp \
	vareditdlg.cpp
if BUILD_UNICODE_CONSOLE
nxmc_CPPFLAGS = -DUNICODE
nxmc_LDADD = ../../../libnetxms/libnetxmsw.la ../../../libnxcl/libnxclw.la ../../../libnxmap/libnxmapw.la ../../../libnxsnmp/libnxsnmpw.la ../libnxmc/libnxmc.la
else
nxmc_LDADD = ../../../libnetxms/libnetxms.la ../../../libnxcl/libnxcl.la ../../../libnxmap/libnxmap.la ../../../libnxsnmp/libnxsnmp.la ../libnxmc/libnxmc.la
endif

EXTRA_DIST = \
	nxmc.dsp \
	nxmc.rc \
	busydlg.h \
	conlog.h \
	ctrlpanel.h \
	frame.h \
	logindlg.h \
	mainfrm.h \
	nxmc.h \
	resource.h \
	srvcfg.h \
	vareditdlg.h
