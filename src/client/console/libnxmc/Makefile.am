INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/client/console/include

lib_LTLIBRARIES = libnxmc.la
libnxmc_la_SOURCES = alarms.cpp events.cpp graph.cpp heading.cpp image.cpp \
		     main.cpp nxview.cpp objseldlg.cpp tools.cpp view.cpp \
		     viewtracker.cpp wxPieCtrl.cpp userSelectionDialog.cpp
libnxmc_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
if BUILD_UNICODE_CONSOLE
libnxmc_la_CPPFLAGS = -DUNICODE
libnxmc_la_LIBADD = ../../../libnetxms/libnetxmsw.la ../../../libnxcl/libnxclw.la
else
libnxmc_la_LIBADD = ../../../libnetxms/libnetxms.la ../../../libnxcl/libnxcl.la
endif

EXTRA_DIST = \
	libnxmc.dsp libnxmc.h nxview.h objseldlg.h heading.h graph.h userSelectionDialog.h
