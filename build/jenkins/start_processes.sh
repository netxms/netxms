#!/bin/sh

echo "Starting netxmsd and nxagentd"

if test "x$BUILD_PREFIX" = "x"; then
	echo "ERROR: build prefix not set"
	exit 1
fi

if test "x$DISABLE_ASAN" = "x0" -a "x$DISABLE_ASAN_ON_NODE" = "x0"; then
	SUFFIX="-asan"
else
	SUFFIX=""
fi
$BUILD_PREFIX/bin/nxdbmgr$SUFFIX -f unlock
$BUILD_PREFIX/bin/nxdbmgr$SUFFIX upgrade
$BUILD_PREFIX/bin/netxmsd$SUFFIX -d -D6
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
