#!/bin/sh

echo "Stopping netxmsd and nxagentd if they are running"

if test "x$BUILD_PREFIX" = "x"; then
	echo "ERROR: build prefix not set"
	exit 1
fi

if ps cax | grep [n]etxmsd; then
    $BUILD_PREFIX/bin/nxadm -c down
fi

if ps cax | grep [n]xagentd && [ -f $BUILD_PREFIX/agent.pid ]; then
    kill `cat $BUILD_PREFIX/agent.pid`
else
    if ps cax | grep [n]xagentd; then
        pkill $command
    fi
fi

sleep 30

if ps cax | grep [n]etxmsd; then
    echo "Command netxmsd is running, but should be already stopped"
    exit 1
fi
