NX_VERSION=$(shell grep AC_INIT ../../../configure.ac | cut -d[ -f3| cut -d] -f1)
PLATFORM=$(shell uname -p)

ifdef SUFIX
	_SUFFIX=$(SUFFIX).
else
	_SUFFIX=
endif

all: package

package:
	sed 's,@version@,$(NX_VERSION),' < pkginfo.in > pkginfo
	rm -rf /tmp/RADnxagent
	pkgmk -o -r / -d /tmp -f Prototype
	pkgtrans /tmp /tmp/RADnxagent.$(NX_VERSION).$(_SUFFIX)$(PLATFORM).pkg RADnxagent
	gzip -f9 /tmp/RADnxagent.$(NX_VERSION).$(_SUFFIX)$(PLATFORM).pkg
	rm -rf /tmp/RADnxagent
	rm pkginfo
