# Build environment for universal Linux agent packages.
#
# Based on PyPA manylinux: an old glibc userland (2.17 for manylinux2014,
# 2.28 for manylinux_2_28) with a modern GCC from devtoolset/gcc-toolset on
# top. Binaries built here run on any distribution with glibc >= that floor.
# All third-party libraries the agent links against are built from pinned
# sources as static, position-independent archives under /opt/<name>, so
# nothing from the base image's own package set ends up in shipped binaries.
#
# Build:
#   docker build -t netxms-agent-builder packages/linux-universal
# For aarch64 (needs qemu binfmt on an x86_64 host, or a native arm64 host):
#   docker build --platform linux/arm64 \
#      --build-arg BASE_IMAGE=quay.io/pypa/manylinux2014_aarch64 \
#      -t netxms-agent-builder:aarch64 packages/linux-universal
# For 32-bit x86 (runs on any x86_64 host, no emulation needed):
#   docker build --platform linux/386 \
#      --build-arg BASE_IMAGE=quay.io/pypa/manylinux2014_i686 \
#      -t netxms-agent-builder:i686 packages/linux-universal
#
# Commands run through manylinux-entrypoint, which switches to the 32-bit
# personality (linux32) on the i686 image so uname and OpenSSL's target
# detection see i686; on other images it is a plain exec.
#
# Pin BASE_IMAGE by digest for reproducible releases.

ARG BASE_IMAGE=quay.io/pypa/manylinux2014_x86_64
FROM ${BASE_IMAGE}

# Symbol version floor that check-elf.sh enforces on build output.
# Must match BASE_IMAGE: manylinux2014 -> 2.17 / 3.4.19, manylinux_2_28 -> 2.28 / 3.4.25
ARG GLIBC_MAX=2.17
ARG GLIBCXX_MAX=3.4.19
ENV NXBUILD_GLIBC_MAX=${GLIBC_MAX} \
    NXBUILD_GLIBCXX_MAX=${GLIBCXX_MAX}

# Third-party library versions and SHA-256 of release tarballs
ARG OPENSSL_VERSION=3.5.9
ARG OPENSSL_SHA256=603f5602e2eef00d77fbd429d34dcd5822bb301757a1bc9cdb24c670f1eb859a
ARG ZLIB_VERSION=1.3.2
ARG ZLIB_SHA256=bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16
ARG EXPAT_VERSION=2.9.0
ARG EXPAT_SHA256=1e6371862cc31999b368c3b89b49994f0677e1bab5f1b2b85ae3741f5d803051
ARG PCRE2_VERSION=10.49
ARG PCRE2_SHA256=53c156e1ba416a20da8e65395daa132da0d80e76910424caca3fcdae7831d384
ARG CURL_VERSION=8.23.0
ARG CURL_SHA256=8dadd7d2536a314f982ad05a651b713f426bf7d02cc2c76cffb9796f2218e6c5
ARG LIBSSH_VERSION=0.12.2
ARG LIBSSH_SHA256=49560f677d96e3706a904ac2de1116e25f3680937d51e5c92198fcba4a1c1e9f

# yum is needed only for flex and two perl modules used by OpenSSL's Configure. CentOS 7
# is EOL and vault.centos.org is unreliable, so use the kernel.org mirror of the vault.
RUN sed -i -e 's#https\?://vault.centos.org/centos/\($releasever\|7\)/#https://archive.kernel.org/centos-vault/7.9.2009/#g' \
           -e 's#https\?://vault.centos.org/altarch/\($releasever\|7\)/#https://archive.kernel.org/centos-vault/altarch/7.9.2009/#g' \
           /etc/yum.repos.d/*.repo && \
    yum -y install perl-IPC-Cmd perl-Time-Piece flex && yum clean all

COPY build-deps.sh /usr/local/sbin/build-deps.sh
RUN OPENSSL_VERSION=${OPENSSL_VERSION} OPENSSL_SHA256=${OPENSSL_SHA256} \
    ZLIB_VERSION=${ZLIB_VERSION} ZLIB_SHA256=${ZLIB_SHA256} \
    EXPAT_VERSION=${EXPAT_VERSION} EXPAT_SHA256=${EXPAT_SHA256} \
    PCRE2_VERSION=${PCRE2_VERSION} PCRE2_SHA256=${PCRE2_SHA256} \
    CURL_VERSION=${CURL_VERSION} CURL_SHA256=${CURL_SHA256} \
    LIBSSH_VERSION=${LIBSSH_VERSION} LIBSSH_SHA256=${LIBSSH_SHA256} \
    manylinux-entrypoint /usr/local/sbin/build-deps.sh

COPY build.sh check-elf.sh /usr/local/sbin/

# Install prefix for the dynamic variant; world-writable so the container can
# run as the invoking user (--user) and leave the mounted tree owned by them.
RUN mkdir -p /opt/netxms /out && chmod 1777 /opt/netxms /out

WORKDIR /src
ENTRYPOINT ["manylinux-entrypoint", "/usr/local/sbin/build.sh"]
