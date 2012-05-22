NX_VERSION=$(shell grep AC_INIT ../../configure.ac | awk -F '[\]\[]' '{ print $$4; }')
PLATFORM=$(shell uname -p)

ifdef SUFIX
	_SUFIX=$(SUFIX).
else
	_SUFIX=
endif

all:
	sed 's,@version@,$(NX_VERSION),' < pkginfo.in > pkginfo
	rm -rf /tmp/RADnxagent
	pkgmk -o -r / -d /tmp -f Prototype
	cd /tmp
	tar -cf - RADnxagent | gzip -9 -c > RADnxagent.$(NX_VERSION).$(_SUFIX)$(PLATFORM).pkg.tar.gz
	cd -
	rm -rf /tmp/RADnxagent
	rm pkginfo
