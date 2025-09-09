#!/bin/sh

echo "Starting netxmsd and nxagentd"

if test "x$BUILD_PREFIX" = "x"; then
    echo "ERROR: build prefix not set"
    exit 1
fi

OS=`uname -s`
case "$OS" in
    FreeBSD)
        PS_OPTIONS="-aex"
        ;;
    OpenBSD)
        PS_OPTIONS="-ax"
        ;;
    *)
        PS_OPTIONS="-ae"
        ;;
esac

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

if test -x "$BINDIR/nxdbmgr" -a -x "$BINDIR/netxmsd"; then
	if $BINDIR/nxdbmgr$SUFFIX $NETXMSD_CONFIG -f unlock | grep 'Unable to determine database syntax'; then
	   $BINDIR/nxdbmgr$SUFFIX $NETXMSD_CONFIG init 
	fi
	$BINDIR/nxdbmgr$SUFFIX $NETXMSD_CONFIG upgrade
	$BINDIR/nxdbmgr$SUFFIX $NETXMSD_CONFIG -E check
	if [ $? -ne 0 ]; then
      echo "Failed to execute nxdbmgr check"
      exit 1
	fi
	ulimit -c unlimited
	$BINDIR/netxmsd$SUFFIX $NETXMSD_CONFIG -d -D6
	SERVER_STARTED=yes
fi
if test -x "$BINDIR/nxagentd"; then
	$BINDIR/nxagentd$SUFFIX $NXAGENTD_CONFIG -d -D6 -p $BUILD_PREFIX/agent.pid
	AGENT_STARTED=yes
fi

# Wait for processes to start
sleep 60

if [ "x$SERVER_STARTED" = "xyes" ]; then
	if ps $PS_OPTIONS -o pid,user,args | grep -v grep | grep $USER | grep $BINDIR/netxmsd; then
	    echo "NetXMS server is running"
	else
	    echo "NetXMS server is not running"
	    exit 1
	fi
fi

if [ "x$AGENT_STARTED" = "xyes" ]; then
	if ps $PS_OPTIONS -o pid,user,args | grep -v grep | grep $USER | grep $BINDIR/nxagentd; then
	    echo "NetXMS agent is running"
	else
	    echo "NetXMS agent is not running"
	    exit 1
	fi
fi
