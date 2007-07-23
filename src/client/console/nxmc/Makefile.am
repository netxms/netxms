SUBDIRS = rc

INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/client/console/include

bin_PROGRAMS = nxmc
nxmc_SOURCES = \
	busydlg.cpp \
	comm.cpp \
	conlog.cpp \
	frame.cpp \
	logindlg.cpp \
	mainfrm.cpp \
	nxmc.cpp \
	plugins.cpp
nxmc_LDADD = ../../../libnetxms/libnetxms.la ../../../libnxcl/libnxcl.la ../../../libnxmap/libnxmap.la ../../../libnxsnmp/libnxsnmp.la ../libnxmc/libnxmc.la

EXTRA_DIST = \
	nxmc.dsp \
	nxmc.rc \
	busydlg.h \
	conlog.h \
	frame.h \
	logindlg.h \
	mainfrm.h \
	nxmc.h \
	resource.h
