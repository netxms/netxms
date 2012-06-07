NX_VERSION=$(shell grep AC_INIT ../../configure.ac | awk -F '[][]' '{ print $$4; }')
PLATFORM=$(shell uname -p)

ifdef SUFIX
	_SUFIX=$(SUFIX).
else
	_SUFIX=
endif

all: prereq package

prereq:
	cp /opt/csw/lib/libcrypto.so.0.9.8 /opt/netxms/lib
	cp /opt/csw/lib/libgcc_s.so.1 /opt/netxms/lib
	cp /opt/csw/lib/libstdc++.so.6 /opt/netxms/lib

package:
	sed 's,@version@,$(NX_VERSION),' < pkginfo.in > pkginfo
	rm -rf /tmp/RADnxagent
	pkgmk -o -r / -d /tmp -f Prototype
	pkgtrans /tmp /tmp/RADnxagent.$(NX_VERSION).$(_SUFIX)$(PLATFORM).pkg RADnxagent
	gzip -f9 /tmp/RADnxagent.$(NX_VERSION).$(_SUFIX)$(PLATFORM).pkg
	rm -rf /tmp/RADnxagent
	rm pkginfo
