# Universal Linux agent build environment

Container-based build of the two universal Linux agent variants:

* **dynamic** — `nxagentd` plus shared libraries and subagents, packed as
  `nxagent-<version>-<suffix>.apkg` (self-extracting installer) and `.tar.gz`
* **static** — single `nxagentd` binary that depends on glibc only, linked by
  `src/agent/core/static_link.sh` and shipped gzip-compressed as
  `nxagentd-<version>-<suffix>-static.gz`

The image is based on PyPA manylinux (CentOS 7 userland, glibc 2.17, GCC 10 from
devtoolset), so the output runs on any distribution with glibc 2.17 or newer. All
third-party libraries (OpenSSL, libcurl, libssh, PCRE2, zlib, expat) are built from
pinned sources as static archives inside the image; nothing from the base image's
package set is linked into shipped binaries.

## Usage

```sh
# build the image once (re-run after changing library versions in the Dockerfile)
docker build -t netxms-agent-builder packages/linux-universal

# build both variants from a clean checkout; results land in ./out
mkdir -p out
docker run --rm --user $(id -u):$(id -g) \
   -v "$PWD:/src" -v "$PWD/out:/out" netxms-agent-builder
```

The source tree is built in place, so use a clean checkout (or `make distclean` first).
`build.sh` runs `build/prepare_release_build.sh` when `/src` is a git checkout and
plain `./reconf` otherwise.

Environment variables accepted by `build.sh` (pass with `-e`):

| Variable | Default | Meaning |
|---|---|---|
| `VARIANTS` | `dynamic static` | which variants to build |
| `SUFFIX` | `linux-<uname -m>` | package name suffix |
| `CONFIGURE_EXTRA` | | extra `configure` arguments for both variants |
| `STATIC_SUBAGENTS` | `default` | `--with-static-subagents-list` for the static variant |

## Other architectures

Use the matching manylinux base image. On an x86_64 host this needs qemu binfmt
(`docker run --privileged --rm tonistiigi/binfmt --install arm64`); on a native
arm64 host no extra setup is needed.

```sh
docker build --platform linux/arm64 \
   --build-arg BASE_IMAGE=quay.io/pypa/manylinux2014_aarch64 \
   -t netxms-agent-builder:aarch64 packages/linux-universal
docker run --rm --platform linux/arm64 --user $(id -u):$(id -g) \
   -v "$PWD:/src" -v "$PWD/out:/out" netxms-agent-builder:aarch64
```

32-bit x86 builds use the i686 image. It runs natively on any x86_64 host; the
`manylinux-entrypoint` wrapper switches to the 32-bit personality so `uname -m`
reports `i686` and the package suffix becomes `linux-i686`.

```sh
docker build --platform linux/386 \
   --build-arg BASE_IMAGE=quay.io/pypa/manylinux2014_i686 \
   -t netxms-agent-builder:i686 packages/linux-universal
docker run --rm --platform linux/386 --user $(id -u):$(id -g) \
   -v "$PWD:/src" -v "$PWD/out:/out" netxms-agent-builder:i686
```

## Raising the glibc floor

To switch to glibc 2.28 (AlmaLinux 8 userland, newer toolchain, drops RHEL 7 /
Ubuntu 16.04 / Debian 9 targets):

```sh
docker build --build-arg BASE_IMAGE=quay.io/pypa/manylinux_2_28_x86_64 \
   --build-arg GLIBC_MAX=2.28 --build-arg GLIBCXX_MAX=3.4.25 ...
```

`GLIBC_MAX` / `GLIBCXX_MAX` are the floors enforced by `check-elf.sh` on every
produced ELF file; the build fails if a binary needs a newer symbol version or
depends on a shared library outside glibc, libstdc++/libgcc and the package itself.
The static variant is checked with `--libc-only`.

## Reproducibility

Pin `BASE_IMAGE` by digest (`quay.io/pypa/manylinux2014_x86_64@sha256:...`) for
release builds. Library tarballs are verified against the SHA-256 values in the
Dockerfile.
