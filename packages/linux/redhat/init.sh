#!/bin/sh

source ./vars.sh

mkdir -p "$DIR_ROOT"
mkdir -p "$DIR_OUT"
cd "$DIR_ROOT"
mkdir -p "$DIR_BUILD" "$DIR_RPMS" "$DIR_SOURCE" "$DIR_SPEC" "$DIR_SRPMS" "$DIR_BUILDROOT"
