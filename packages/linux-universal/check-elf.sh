#!/bin/bash
#
# Verify that ELF files are portable across Linux distributions:
#   - no glibc symbol version above the floor
#   - no libstdc++/libgcc symbol version above the floor (dynamic variant)
#   - no shared library dependencies outside glibc, libstdc++/libgcc and
#     the package's own libraries
#
# Usage: check-elf.sh [options] <file>...
#   --glibc-max VER     highest allowed GLIBC_ symbol version (default $NXBUILD_GLIBC_MAX)
#   --glibcxx-max VER   highest allowed GLIBCXX_ symbol version (default $NXBUILD_GLIBCXX_MAX)
#   --libc-only         allow only glibc as a dynamic dependency (static agent variant)
#   --libdir DIR        shared objects in DIR are allowed dependencies (dynamic variant)
#
# Exit status is non-zero if any file fails.
#

set -u

GLIBC_MAX="${NXBUILD_GLIBC_MAX:-}"
GLIBCXX_MAX="${NXBUILD_GLIBCXX_MAX:-}"
LIBC_ONLY=no
LIBDIR=""

while [ $# -gt 0 ]; do
   case "$1" in
      --glibc-max) GLIBC_MAX="$2"; shift 2 ;;
      --glibcxx-max) GLIBCXX_MAX="$2"; shift 2 ;;
      --libc-only) LIBC_ONLY=yes; shift ;;
      --libdir) LIBDIR="$2"; shift 2 ;;
      --) shift; break ;;
      -*) echo "Unknown option $1" >&2; exit 2 ;;
      *) break ;;
   esac
done

if [ $# -eq 0 ] || [ -z "$GLIBC_MAX" ]; then
   echo "Usage: check-elf.sh --glibc-max VER [--glibcxx-max VER] [--libc-only] [--libdir DIR] <file>..." >&2
   exit 2
fi

GLIBC_LIBS="libc.so.6 libm.so.6 libpthread.so.0 libdl.so.2 librt.so.1 libresolv.so.2 libutil.so.1 libcrypt.so.1"
GLIBC_LIBS="$GLIBC_LIBS ld-linux-x86-64.so.2 ld-linux-aarch64.so.1 ld-linux-armhf.so.3 ld-linux.so.2 ld64.so.2 ld64.so.1"
CXX_LIBS="libstdc++.so.6 libgcc_s.so.1"

# Returns 0 if $1 (a version like 2.17 or 3.4.19) is <= $2
version_le()
{
   [ "$(printf '%s\n%s\n' "$1" "$2" | sort -V | head -1)" = "$1" ]
}

# Highest version with the given prefix referenced by the file, empty if none
max_version()
{
   readelf -V "$1" 2>/dev/null | grep -o "Name: $2[0-9.]*" | sed "s/Name: $2//" | sort -V | tail -1
}

allowed_lib()
{
   local lib="$1" l
   for l in $GLIBC_LIBS; do [ "$l" = "$lib" ] && return 0; done
   if [ "$LIBC_ONLY" != "yes" ]; then
      for l in $CXX_LIBS; do [ "$l" = "$lib" ] && return 0; done
      [ -n "$LIBDIR" ] && [ -e "$LIBDIR/$lib" ] && return 0
   fi
   return 1
}

rc=0
for f in "$@"; do
   [ -L "$f" ] && continue
   head -c 4 "$f" 2>/dev/null | grep -q 'ELF' || continue
   errors=""

   v=$(max_version "$f" GLIBC_)
   if [ -n "$v" ] && ! version_le "$v" "$GLIBC_MAX"; then
      errors="$errors\n   needs GLIBC_$v (floor is GLIBC_$GLIBC_MAX)"
   fi

   for prefix in GLIBCXX_ CXXABI_; do
      v=$(max_version "$f" $prefix)
      if [ -n "$v" ]; then
         if [ "$LIBC_ONLY" = "yes" ]; then
            errors="$errors\n   needs $prefix$v but must depend on glibc only"
         elif [ "$prefix" = "GLIBCXX_" ] && [ -n "$GLIBCXX_MAX" ] && ! version_le "$v" "$GLIBCXX_MAX"; then
            errors="$errors\n   needs GLIBCXX_$v (floor is GLIBCXX_$GLIBCXX_MAX)"
         fi
      fi
   done

   for lib in $(readelf -d "$f" 2>/dev/null | grep -o 'NEEDED.*\[.*\]' | sed 's/.*\[\(.*\)\]/\1/'); do
      allowed_lib "$lib" || errors="$errors\n   depends on $lib"
   done

   if [ -n "$errors" ]; then
      printf "FAIL %s%b\n" "$f" "$errors"
      rc=1
   fi
done

[ $rc -eq 0 ] && echo "check-elf: all $# files OK (GLIBC <= $GLIBC_MAX${GLIBCXX_MAX:+, GLIBCXX <= $GLIBCXX_MAX})"
exit $rc
