#!/bin/sh

VERSION=$1
if [ "x$VERSION" = "x" ]; then
	echo "Usage: $0 <version>"
	exit
fi

ARCH=`uname -m`
if [ "x$ARCH" = "xx86_64" ]; then
	ARCH="amd64"
fi
if [ "x$ARCH" = "xi686" ]; then
	ARCH="i386"
fi

# netxms-base
mkdir netxms-base
mkdir netxms-base/DEBIAN
mkdir netxms-base/usr
mkdir netxms-base/usr/bin
mkdir netxms-base/usr/lib
cat netxms-base.control | sed "s/@arch@/$ARCH/" | sed "s/@version@/$VERSION/" > netxms-base/DEBIAN/control
cp -P /usr/lib/libnetxms.so* netxms-base/usr/lib/
cp -P /usr/lib/libnxjansson.so* netxms-base/usr/lib/
cp -P /usr/lib/libnxsqlite.so* netxms-base/usr/lib/
cp -P /usr/lib/libnxtre.so* netxms-base/usr/lib/

# netxms-agent
mkdir netxms-agent
mkdir netxms-agent/DEBIAN
mkdir netxms-agent/etc
mkdir netxms-agent/etc/init.d
mkdir netxms-agent/usr
mkdir netxms-agent/usr/bin
mkdir netxms-agent/usr/lib
mkdir netxms-agent/usr/lib/netxms
mkdir netxms-agent/usr/lib/netxms/dbdrv
mkdir netxms-agent/var
mkdir netxms-agent/var/netxms
cat netxms-agent.control | sed "s/@arch@/$ARCH/" | sed "s/@version@/$VERSION/" > netxms-agent/DEBIAN/control
cp netxms-agent.preinst netxms-agent/DEBIAN/preinst
cp netxms-agent.postinst netxms-agent/DEBIAN/postinst
cp netxms-agent.prerm netxms-agent/DEBIAN/prerm
cp netxms-agent.postrm netxms-agent/DEBIAN/postrm
chmod 755 netxms-agent/DEBIAN/*
cp /usr/bin/nxagentd netxms-agent/usr/bin/
cp /usr/bin/nxappget netxms-agent/usr/bin/
cp /usr/bin/nxapush netxms-agent/usr/bin/
cp /usr/bin/nxevent netxms-agent/usr/bin/
cp /usr/bin/nxpush netxms-agent/usr/bin/
cp /usr/lib/netxms/*.nsm netxms-agent/usr/lib/netxms/
cp /usr/lib/netxms/dbdrv/sqlite.ddr netxms-agent/usr/lib/netxms/dbdrv/
cp -P /usr/lib/libappagent.so* netxms-agent/usr/lib/
cp -P /usr/lib/libnxdb.so* netxms-agent/usr/lib/
cp -P /usr/lib/libnxlp.so* netxms-agent/usr/lib/
cp -P /usr/lib/libnxsnmp.so* netxms-agent/usr/lib/
cp -P /usr/lib/libnsm_* netxms-agent/usr/lib/
cp -P /usr/lib/libnxddr_sqlite.so* netxms-agent/usr/lib/
cp -P /usr/lib/libnxmap.so* netxms-agent/usr/lib/
cp -P /usr/lib/libnxclient.so* netxms-agent/usr/lib/
cp ../../../../contrib/startup/debian/nxagentd netxms-agent/etc/init.d/
cp ../../../../contrib/nxagentd.conf-dist netxms-agent/etc
chmod 755 netxms-agent/etc/init.d/nxagentd

# netxms-server
mkdir netxms-server
mkdir netxms-server/DEBIAN
mkdir netxms-server/etc
mkdir netxms-server/etc/init.d
mkdir netxms-server/usr
mkdir netxms-server/usr/bin
mkdir netxms-server/usr/lib
mkdir netxms-server/usr/lib/netxms
mkdir netxms-server/usr/lib/netxms/ndd
mkdir netxms-server/usr/share
mkdir netxms-server/usr/share/netxms
mkdir netxms-server/usr/share/netxms/images
mkdir netxms-server/usr/share/netxms/mibs
mkdir netxms-server/usr/share/netxms/sql
mkdir netxms-server/var
mkdir netxms-server/var/netxms
cat netxms-server.control | sed "s/@arch@/$ARCH/" | sed "s/@version@/$VERSION/" > netxms-server/DEBIAN/control
cp netxms-server.preinst netxms-server/DEBIAN/preinst
cp netxms-server.postinst netxms-server/DEBIAN/postinst
cp netxms-server.prerm netxms-server/DEBIAN/prerm
cp netxms-server.postrm netxms-server/DEBIAN/postrm
chmod 755 netxms-server/DEBIAN/*
cp /usr/bin/netxmsd netxms-server/usr/bin/
cp /usr/bin/nxadm netxms-server/usr/bin/
cp /usr/bin/nxdbmgr netxms-server/usr/bin/
cp /usr/bin/nxencpasswd netxms-server/usr/bin/
cp /usr/bin/nxget netxms-server/usr/bin/
cp /usr/bin/nxmibc netxms-server/usr/bin/
cp /usr/bin/nxscript netxms-server/usr/bin/
cp /usr/bin/nxsnmpget netxms-server/usr/bin/
cp /usr/bin/nxsnmpset netxms-server/usr/bin/
cp /usr/bin/nxsnmpwalk netxms-server/usr/bin/
cp /usr/bin/nxupload netxms-server/usr/bin/
cp /usr/lib/netxms/*.hdlink netxms-server/usr/lib/netxms/
cp /usr/lib/netxms/ndd/*.ndd netxms-server/usr/lib/netxms/ndd/
cp -P /usr/lib/libavaya-ers.so* netxms-server/usr/lib/
cp -P /usr/lib/libcisco.so* netxms-server/usr/lib/
cp -P /usr/lib/libnxcore.so* netxms-server/usr/lib/
cp -P /usr/lib/libnxsl.so* netxms-server/usr/lib/
cp -P /usr/lib/libnxsms_dummy.so* netxms-server/usr/lib/
cp -P /usr/lib/libnxsms_generic.so* netxms-server/usr/lib/
cp -P /usr/lib/libnxsms_nxagent.so* netxms-server/usr/lib/
cp -P /usr/lib/libnxsms_portech.so* netxms-server/usr/lib/
cp -P /usr/lib/libnxsms_websms.so* netxms-server/usr/lib/
cp -P /usr/lib/libnxsrv.so* netxms-server/usr/lib/
cp -P /usr/lib/libstrophe.so* netxms-server/usr/lib/
cp /usr/share/netxms/mibs/*.txt netxms-server/usr/share/netxms/mibs/
cp /usr/share/netxms/sql/* netxms-server/usr/share/netxms/sql/
cp ../../../../contrib/startup/debian/netxmsd netxms-server/etc/init.d/
cp ../../../../contrib/netxmsd.conf-dist netxms-server/etc
cp ../../../../images/* netxms-server/usr/share/netxms/images/
rm -f netxms-server/usr/share/netxms/images/Makefile*
chmod 755 netxms-server/etc/init.d/netxmsd

# netxms-server-pgsql
mkdir netxms-server-pgsql
mkdir netxms-server-pgsql/DEBIAN
mkdir netxms-server-pgsql/usr
mkdir netxms-server-pgsql/usr/lib
mkdir netxms-server-pgsql/usr/lib/netxms
mkdir netxms-server-pgsql/usr/lib/netxms/dbdrv
cat netxms-server-pgsql.control | sed "s/@arch@/$ARCH/" | sed "s/@version@/$VERSION/" > netxms-server-pgsql/DEBIAN/control
cp /usr/lib/netxms/dbdrv/pgsql.ddr netxms-server-pgsql/usr/lib/netxms/dbdrv/
cp -P /usr/lib/libnxddr_pgsql.so* netxms-server-pgsql/usr/lib/

# netxms-server-mysql
mkdir netxms-server-mysql
mkdir netxms-server-mysql/DEBIAN
mkdir netxms-server-mysql/usr
mkdir netxms-server-mysql/usr/lib
mkdir netxms-server-mysql/usr/lib/netxms
mkdir netxms-server-mysql/usr/lib/netxms/dbdrv
cat netxms-server-mysql.control | sed "s/@arch@/$ARCH/" | sed "s/@version@/$VERSION/" > netxms-server-mysql/DEBIAN/control
cp /usr/lib/netxms/dbdrv/mysql.ddr netxms-server-mysql/usr/lib/netxms/dbdrv/
cp -P /usr/lib/libnxddr_mysql.so* netxms-server-mysql/usr/lib/

# netxms-server-oracle
mkdir netxms-server-oracle
mkdir netxms-server-oracle/DEBIAN
mkdir netxms-server-oracle/usr
mkdir netxms-server-oracle/usr/lib
mkdir netxms-server-oracle/usr/lib/netxms
mkdir netxms-server-oracle/usr/lib/netxms/dbdrv
cat netxms-server-oracle.control | sed "s/@arch@/$ARCH/" | sed "s/@version@/$VERSION/" > netxms-server-oracle/DEBIAN/control
cp /usr/lib/netxms/dbdrv/oracle.ddr netxms-server-oracle/usr/lib/netxms/dbdrv/
cp -P /usr/lib/libnxddr_oracle.so* netxms-server-oracle/usr/lib/

# netxms-server-odbc
mkdir netxms-server-odbc
mkdir netxms-server-odbc/DEBIAN
mkdir netxms-server-odbc/usr
mkdir netxms-server-odbc/usr/lib
mkdir netxms-server-odbc/usr/lib/netxms
mkdir netxms-server-odbc/usr/lib/netxms/dbdrv
cat netxms-server-odbc.control | sed "s/@arch@/$ARCH/" | sed "s/@version@/$VERSION/" > netxms-server-odbc/DEBIAN/control
cp /usr/lib/netxms/dbdrv/odbc.ddr netxms-server-odbc/usr/lib/netxms/dbdrv/
cp -P /usr/lib/libnxddr_odbc.so* netxms-server-odbc/usr/lib/
