NX_VERSION=$(shell grep AC_INIT ../../../configure.ac | awk -F '[][]' '{ print $$4; }')
PLATFORM=$(shell uname -p)

ifdef SUFIX
	_SUFFIX=$(SUFFIX).
else
	_SUFFIX=
endif

all: package

package:
	mkdir -p /opt/netxms/svc
	cp files/netxmsd.xml /var/svc/manifest/application/management/
	cp ../agent/files/nxagentd.xml /var/svc/manifest/application/management/
	cp files/netxmsd /opt/netxms/svc/
	cp ../agent/files/nxagentd /opt/netxms/svc/
	sed 's,@version@,$(NX_VERSION),' < pkginfo.in > pkginfo
	rm -rf /tmp/RADnetxms
	pkgmk -o -r / -d /tmp -f Prototype
	pkgtrans /tmp /tmp/RADnetxms.$(NX_VERSION).$(_SUFFIX)$(PLATFORM).pkg RADnetxms
	gzip -f9 /tmp/RADnetxms.$(NX_VERSION).$(_SUFFIX)$(PLATFORM).pkg
	rm -rf /tmp/RADnetxms
	rm pkginfo
	rm -f /var/svc/manifest/application/management/nxagentd.xml
	rm -f /var/svc/manifest/application/management/netxmsd.xml
	rm -f /opt/netxms/svc/nxagentd
	rm -f /opt/netxms/svc/netxmsd
	rmdir /opt/netxms/svc
