#!/bin/sh

echo "Starting netxmsd and nxagentd"

if test "x$BUILD_PREFIX" = "x"; then
	echo "ERROR: build prefix not set"
	exit 1
fi

$BUILD_PREFIX/bin/nxdbmgr-asan -f unlock
$BUILD_PREFIX/bin/nxdbmgr-asan upgrade
$BUILD_PREFIX/bin/netxmsd-asan -d -D6
$BUILD_PREFIX/bin/nxagentd -d -D6 -p $BUILD_PREFIX/agent.pid

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
