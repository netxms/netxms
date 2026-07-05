bin_PROGRAMS = nxsagent

nxsagent_SOURCES = main.cpp png.cpp screenshot.cpp
nxsagent_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxsagent_LDFLAGS = @EXEC_LDFLAGS@
nxsagent_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @PNG_LIBS@ @X11_LIBS@ @EXEC_LIBS@

if HAVE_SDBUS
nxsagent_SOURCES += screenshot_wayland.cpp
nxsagent_LDADD += @SYSTEMD_LIBS@
endif

EXTRA_DIST = \
	nxsagent.h
