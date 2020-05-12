#!/bin/sh

echo "Stopping netxmsd and nxagentd if they are running"

if test "x$BUILD_PREFIX" = "x"; then
	echo "ERROR: build prefix not set"
	exit 1
fi

if ps cax | grep [n]etxmsd; then
    if test -x $BUILD_PREFIX/bin/nxadm; then
        $BUILD_PREFIX/bin/nxadm -c down
    else
        pid=`ps cax | grep [n]etxmsd | xargs | cut -d ' ' -f 1`
        kill $pid
    fi
fi

if ps cax | grep [n]xagentd; then
    if [ -f $BUILD_PREFIX/agent.pid ]; then
        pid=`cat $BUILD_PREFIX/agent.pid`
    else
        pid=`ps cax | grep [n]xagentd | xargs | cut -d ' ' -f 1`
    fi
    kill $pid
fi

sleep 30

if ps cax | grep [n]etxmsd; then
    echo "Command netxmsd is running, but should be already stopped"
    exit 1
fi
