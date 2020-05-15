#!/bin/sh

echo "Starting netxmsd and nxagentd"

if test "x$BUILD_PREFIX" = "x"; then
    echo "ERROR: build prefix not set"
    exit 1
fi

OS=`uname -s`
if test "$OS" = "FreeBSD"; then
    PS_OPTIONS="-aex"
else
    PS_OPTIONS="-ae"
fi

BINDIR="$BUILD_PREFIX/bin"
USER=`whoami`

if test "x$DISABLE_ASAN" = "x0" -a "x$DISABLE_ASAN_ON_NODE" = "x0"; then
    SUFFIX="-asan"
else
    SUFFIX=""
fi

if test -f "$BUILD_PREFIX/etc/jenkins/netxmsd.conf"; then
	NETXMSD_CONFIG="-c $BUILD_PREFIX/etc/jenkins/netxmsd.conf"
fi
if test -f "$BUILD_PREFIX/etc/jenkins/nxagentd.conf"; then
	NXAGENTD_CONFIG="-c $BUILD_PREFIX/etc/jenkins/nxagentd.conf"
fi

$BINDIR/nxdbmgr$SUFFIX $NETXMSD_CONFIG -f unlock
$BINDIR/nxdbmgr$SUFFIX $NETXMSD_CONFIG upgrade
$BINDIR/netxmsd$SUFFIX $NETXMSD_CONFIG -d -D6
$BINDIR/nxagentd $NXAGENTD_CONFIG -d -D6 -p $BUILD_PREFIX/agent.pid

# Wait for processes to start
sleep 60

if ps $PS_OPTIONS -o pid,user,args | grep -v grep | grep $USER | grep $BINDIR/netxmsd; then
    echo "NetXMS server is running"
else
    echo "NetXMS server is not running"
    exit 1
fi

if ps $PS_OPTIONS -o pid,user,args | grep -v grep | grep $USER | grep $BINDIR/nxagentd; then
    echo "NetXMS agent is running"
else
    echo "NetXMS agent is not running"
    exit 1
fi
