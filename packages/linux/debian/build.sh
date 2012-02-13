#!/bin/sh

dpkg-deb --build netxms-base output &&
dpkg-deb --build netxms-agent output &&
dpkg-deb --build netxms-server output &&
dpkg-deb --build netxms-server-mysql output &&
dpkg-deb --build netxms-server-pgsql output &&
dpkg-deb --build netxms-server-odbc output &&
echo "Done."
