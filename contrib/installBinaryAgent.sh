#!/bin/sh

# $Id$

# Copyright (c) 2007, NetXMS Team
#
# All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name of the <ORGANIZATION> nor the names of its contributors
#   may be used to endorse or promote products derived from this software
#   without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

prefix=/opt/netxms
config=/etc/nxagentd.conf
log=/tmp/nxagentbinupdate.log
platform=`uname -s`

###############################################################################
#
# Main code
#

rm -f $log

# process args
while test "x$1" != "x"; do
	name=`echo $1|cut -d= -f1`
	val=`echo $1|cut -d= -f2`
	eval $name=$val
	shift
done

cd `dirname $0`
name=`ls nxagent-*.tar.gz 2>/dev/null|sed s',\.tar\.gz$,,'`
if test "x$name" = "x"; then
	echo invalid package >>$log
	exit 1
fi

rm -rf tmp
mkdir tmp && cd tmp

gzip -dc ../$name.tar.gz | tar xf - 2>/dev/null
if test $? != 0; then
	echo Unable to unpack >>$log
	exit 2
fi

pids=`ps -e | grep nxagentd | grep -v grep | awk '{ print $1; }'`

if test "x$pids" != "x"; then
	# ask nxagentd gently
	kill $pids >>$log 2>&1
	# wait a few seconds and smash it down
	sleep 15 && kill -9 $pids >>$log 2>&1
fi

# install new files
mkdir -p $prefix 2>/dev/null
if test "x$platform" = "xAIX"; then
	slibclean
fi
tar cf - . | ( cd $prefix ; tar xf - )
if test $? != 0; then
	echo unable to copy new files to $prefix >>$log
	exit 3
fi

# show config
echo "config ($config):" >>$log
cat $config >>$log 2>&1
echo "---------------" >>$log

# and restart
echo "Starting agent: $prefix/bin/nxagentd -d -c $config" >>$log
$prefix/bin/nxagentd -d -c $config >>$log 2>&1
ret=$?
if test $ret != 0; then
	echo nxagentd not started \($ret\) >>$log
	exit 5
fi
