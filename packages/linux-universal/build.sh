#!/bin/bash
#
# Build universal Linux agent packages inside the builder container.
#
# Expects a clean NetXMS source tree mounted at /src and writes results to /out:
#   nxagent-<version>-<suffix>.apkg      self-extracting installer (dynamic variant)
#   nxagent-<version>-<suffix>.tar.gz    bin/ and lib/ of the dynamic variant
#   nxagentd-<version>-<suffix>-static   single static binary (depends on glibc only)
#
# Environment:
#   VARIANTS      "dynamic static" (default) or a subset
#   SUFFIX        package suffix, default Linux-<machine>
#   CONFIGURE_EXTRA  extra arguments passed to both configure runs
#   STATIC_SUBAGENTS  --with-static-subagents-list value for the static variant
#

set -euo pipefail

SRC=/src
OUT=/out
PREFIX=/opt/netxms
NP=$(nproc)
VARIANTS="${VARIANTS:-dynamic static}"
SUFFIX="${SUFFIX:-Linux-$(uname -m)}"
CONFIGURE_EXTRA="${CONFIGURE_EXTRA:-}"
STATIC_SUBAGENTS="${STATIC_SUBAGENTS:-default}"

CHECK_ELF=/usr/local/sbin/check-elf.sh

# All third-party libraries live in /opt/<name> as static archives built by build-deps.sh
DEPS="--with-openssl=/opt/openssl --with-curl=/opt/libcurl --with-pcre2=/opt/pcre --with-zlib=/opt/zlib \
      --with-libexpat=/opt/expat --with-libssh=/opt/libssh --with-internal-libjansson --with-internal-sqlite"

cd $SRC
if [ ! -r configure.ac ]; then
   echo "No NetXMS source tree at $SRC (mount it with -v <tree>:/src)" >&2
   exit 1
fi

if [ -d .git ]; then
   ./build/prepare_release_build.sh
else
   if [ ! -r build/netxms-build-tag.properties ]; then
      echo "Source tree is not a git checkout and has no build/netxms-build-tag.properties; run ./init-source-tree first" >&2
      exit 1
   fi
   ./reconf
fi
VERSION=$(grep NETXMS_VERSION= build/netxms-build-tag.properties | cut -d= -f2)
echo "=== Building NetXMS agent $VERSION for $SUFFIX (variants: $VARIANTS)"

build_dynamic()
{
   echo "=== Dynamic variant: configure"
   ./configure --prefix=$PREFIX --with-agent $DEPS --disable-lmsensors --disable-lldpd --enable-release-build $CONFIGURE_EXTRA
   echo "=== Dynamic variant: make"
   make -j$NP
   rm -rf $PREFIX/bin $PREFIX/lib
   make install
   strip --strip-unneeded $PREFIX/bin/* $PREFIX/lib/*.so* $PREFIX/lib/netxms/*.so* 2>/dev/null || true

   echo "=== Dynamic variant: verifying portability"
   $CHECK_ELF --libdir $PREFIX/lib $PREFIX/bin/* $PREFIX/lib/*.so* $PREFIX/lib/netxms/*.so*

   echo "=== Dynamic variant: packaging"
   tools/create_bin_apkg.sh $PREFIX $VERSION $SUFFIX
   mv nxagent-$VERSION-$SUFFIX.apkg $OUT/
   tar -C $PREFIX -czf $OUT/nxagent-$VERSION-$SUFFIX.tar.gz bin lib
}

build_static()
{
   echo "=== Static variant: configure"
   ./configure --prefix=$PREFIX --with-static-agent --with-static-subagents-list="$STATIC_SUBAGENTS" $DEPS \
      --disable-lmsensors --disable-lldpd --enable-release-build $CONFIGURE_EXTRA
   echo "=== Static variant: make"
   make -j$NP
   echo "=== Static variant: final link"
   (cd src/agent/core && sh static_link.sh)
   strip --strip-unneeded src/agent/core/nxagentd

   echo "=== Static variant: verifying portability"
   $CHECK_ELF --libc-only src/agent/core/nxagentd

   cp src/agent/core/nxagentd $OUT/nxagentd-$VERSION-$SUFFIX-static
}

first=yes
for v in $VARIANTS; do
   [ $first = yes ] || make distclean >/dev/null
   first=no
   case "$v" in
      dynamic) build_dynamic ;;
      static) build_static ;;
      *) echo "Unknown variant $v" >&2; exit 1 ;;
   esac
done

echo "=== Done:"
ls -l $OUT
