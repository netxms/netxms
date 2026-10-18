#!/bin/bash
#
# Build static, position-independent third-party libraries for the universal
# Linux agent into /opt/<name>. Run once during image build; versions and
# checksums come from the Dockerfile.
#

set -euo pipefail

WORK=/tmp/deps
mkdir -p $WORK
cd $WORK

NP=$(nproc)

fetch()
{
   local url="$1" file="$2" sha="$3"
   curl -sSfL --retry 5 -o "$file" "$url"
   echo "$sha  $file" | sha256sum -c -
}

### zlib
fetch "https://github.com/madler/zlib/releases/download/v${ZLIB_VERSION}/zlib-${ZLIB_VERSION}.tar.gz" zlib.tar.gz "$ZLIB_SHA256"
tar xzf zlib.tar.gz
(cd zlib-${ZLIB_VERSION} &&
   CFLAGS="-O2 -fPIC" ./configure --static --prefix=/opt/zlib &&
   make -j$NP && make install)

### expat
fetch "https://github.com/libexpat/libexpat/releases/download/R_${EXPAT_VERSION//./_}/expat-${EXPAT_VERSION}.tar.xz" expat.tar.xz "$EXPAT_SHA256"
tar xJf expat.tar.xz
(cd expat-${EXPAT_VERSION} &&
   ./configure --prefix=/opt/expat --disable-shared --enable-static --with-pic \
      --without-docbook --without-examples --without-tests --without-xmlwf &&
   make -j$NP && make install)

### PCRE2 (8-bit for narrow strings, 32-bit for wchar_t on Linux)
fetch "https://github.com/PCRE2Project/pcre2/releases/download/pcre2-${PCRE2_VERSION}/pcre2-${PCRE2_VERSION}.tar.bz2" pcre2.tar.bz2 "$PCRE2_SHA256"
tar xjf pcre2.tar.bz2
(cd pcre2-${PCRE2_VERSION} &&
   ./configure --prefix=/opt/pcre --disable-shared --enable-static --with-pic \
      --enable-pcre2-8 --enable-pcre2-16 --enable-pcre2-32 --enable-unicode --disable-pcre2grep-libz --disable-pcre2grep-libbz2 &&
   make -j$NP && make install)

### OpenSSL
fetch "https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION}/openssl-${OPENSSL_VERSION}.tar.gz" openssl.tar.gz "$OPENSSL_SHA256"
tar xzf openssl.tar.gz
(cd openssl-${OPENSSL_VERSION} &&
   ./Configure --prefix=/opt/openssl --libdir=lib --openssldir=/opt/openssl/ssl \
      no-shared no-module no-tests no-docs -fPIC &&
   make -j$NP && make install_sw)

### libcurl
fetch "https://curl.se/download/curl-${CURL_VERSION}.tar.xz" curl.tar.xz "$CURL_SHA256"
tar xJf curl.tar.xz
(cd curl-${CURL_VERSION} &&
   PKG_CONFIG_PATH=/opt/openssl/lib/pkgconfig:/opt/zlib/lib/pkgconfig LIBS="-ldl -lpthread" \
   ./configure --prefix=/opt/libcurl --disable-shared --enable-static --with-pic \
      --with-openssl=/opt/openssl --with-zlib=/opt/zlib --with-ca-fallback \
      --without-libpsl --without-libidn2 --without-brotli --without-zstd --without-nghttp2 --without-libssh2 \
      --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp \
      --disable-pop3 --disable-imap --disable-smb --disable-gopher --disable-mqtt \
      --disable-manual --disable-docs --disable-ntlm &&
   make -j$NP && make install)

### libssh (client only; used by ssh and netconf subagents)
fetch "https://www.libssh.org/files/${LIBSSH_VERSION%.*}/libssh-${LIBSSH_VERSION}.tar.xz" libssh.tar.xz "$LIBSSH_SHA256"
tar xJf libssh.tar.xz
(cd libssh-${LIBSSH_VERSION} && mkdir build && cd build &&
   cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/libssh -DCMAKE_INSTALL_LIBDIR=lib \
      -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
      -DOPENSSL_ROOT_DIR=/opt/openssl -DZLIB_ROOT=/opt/zlib \
      -DWITH_SERVER=OFF -DWITH_GSSAPI=OFF -DWITH_EXAMPLES=OFF -DUNIT_TESTING=OFF &&
   make -j$NP && make install)

cd /
rm -rf $WORK
