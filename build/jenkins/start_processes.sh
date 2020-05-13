#!/bin/sh

echo "Starting netxmsd and nxagentd"

if test "x$BUILD_PREFIX" = "x"; then
	echo "ERROR: build prefix not set"
	exit 1
fi

if test "$DISABLE_ASAN" = "0"; then
   $BUILD_PREFIX/bin/nxdbmgr-asan -f unlock
   $BUILD_PREFIX/bin/nxdbmgr-asan upgrade
   $BUILD_PREFIX/bin/netxmsd-asan -d -D6
   $BUILD_PREFIX/bin/nxagentd -d -D6 -p $BUILD_PREFIX/agent.pid
else
   $BUILD_PREFIX/bin/nxdbmgr -f unlock
   $BUILD_PREFIX/bin/nxdbmgr upgrade
   $BUILD_PREFIX/bin/netxmsd -d -D6
   $BUILD_PREFIX/bin/nxagentd -d -D6 -p $BUILD_PREFIX/agent.pid
fi

#Wait process to start
sleep 60

if ps cax | grep [n]etxmsd; then
    echo "Command netxmsd is running"
else
    echo "Command netxmsd is not running"
    exit 1
fi

if ps cax | grep [n]xagentd; then
    echo "Command nxagentd is running"
else
    echo "Command nxagentd is not running"
    exit 1
fi
