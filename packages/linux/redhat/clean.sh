#!/bin/bash

source ./vars.sh

[ -e "$DIR_ROOT" ] && cd "$DIR_ROOT" || exit
rm -Rf "$DIR_BUILD" "$DIR_RPMS" "$DIR_SOURCE" "$DIR_SPEC" "$DIR_SRPMS" "$DIR_BUILDROOT"
cd ..
[ -z "`ls "$DIR_ROOT"`" ] && rm -R "$DIR_ROOT"
[ -z "`ls "$DIR_OUT"`" ] && rm -R "$DIR_OUT"
