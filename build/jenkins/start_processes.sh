#!/bin/sh

echo "Starting netxmsd and nxagentd"

if test "x$BUILD_PREFIX" = "x"; then
	echo "ERROR: build prefix not set"
	exit 1
fi

BINDIR="$BUILD_PREFIX/bin"
USER=`whoami`

if test "x$DISABLE_ASAN" = "x0" -a "x$DISABLE_ASAN_ON_NODE" = "x0"; then
	SUFFIX="-asan"
else
	SUFFIX=""
fi

$BINDIR/nxdbmgr$SUFFIX -f unlock
$BINDIR/nxdbmgr$SUFFIX upgrade
$BINDIR/netxmsd$SUFFIX -d -D6
$BINDIR/nxagentd -d -D6 -p $BUILD_PREFIX/agent.pid

# Wait for processes to start
sleep 60

if ps -ae -o pid,user,args | grep -v grep | grep $USER | grep $BINDIR/netxmsd; then
    echo "NetXMS server is running"
else
    echo "NetXMS server is not running"
    exit 1
fi

if ps -ae -o pid,user,args | grep -v grep | grep $USER | grep $BINDIR/nxagentd; then
    echo "NetXMS agent is running"
else
    echo "NetXMS agent is not running"
    exit 1
fi
