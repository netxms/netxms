NX_VERSION=$(shell grep AC_INIT ../../../configure.ac | awk -F '[][]' '{ print $$4; }')
PLATFORM=$(shell uname -p)

ifdef SUFIX
	_SUFFIX=$(SUFFIX).
else
	_SUFFIX=
endif

all: package

package:
	sed 's,@version@,$(NX_VERSION),' < pkginfo.in > pkginfo
	rm -rf /tmp/RADnetxms
	pkgmk -o -r / -d /tmp -f Prototype
	pkgtrans /tmp /tmp/RADnetxms.$(NX_VERSION).$(_SUFFIX)$(PLATFORM).pkg RADnetxms
	gzip -f9 /tmp/RADnetxms.$(NX_VERSION).$(_SUFFIX)$(PLATFORM).pkg
	rm -rf /tmp/RADnetxms
	rm pkginfo
