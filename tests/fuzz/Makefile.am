# Fuzz targets (libFuzzer). Built only when tree is configured with --enable-fuzzer.

if ENABLE_FUZZER
noinst_PROGRAMS = fuzz-nxcp-decrypt fuzz-nxcp-deserialize fuzz-nxcp-dump fuzz-nxcp-receiver fuzz-nxcp-seedgen
endif

FUZZ_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
FUZZ_LDFLAGS = -fsanitize=fuzzer -no-install
FUZZ_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la

if USE_INTERNAL_JANSSON
FUZZ_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
FUZZ_LDADD += -ljansson
endif

fuzz_nxcp_decrypt_SOURCES = fuzz-nxcp-decrypt.cpp
fuzz_nxcp_decrypt_CPPFLAGS = $(FUZZ_CPPFLAGS)
fuzz_nxcp_decrypt_LDFLAGS = $(FUZZ_LDFLAGS)
fuzz_nxcp_decrypt_LDADD = $(FUZZ_LDADD)

fuzz_nxcp_deserialize_SOURCES = fuzz-nxcp-deserialize.cpp
fuzz_nxcp_deserialize_CPPFLAGS = $(FUZZ_CPPFLAGS)
fuzz_nxcp_deserialize_LDFLAGS = $(FUZZ_LDFLAGS)
fuzz_nxcp_deserialize_LDADD = $(FUZZ_LDADD)

fuzz_nxcp_dump_SOURCES = fuzz-nxcp-dump.cpp
fuzz_nxcp_dump_CPPFLAGS = $(FUZZ_CPPFLAGS)
fuzz_nxcp_dump_LDFLAGS = $(FUZZ_LDFLAGS)
fuzz_nxcp_dump_LDADD = $(FUZZ_LDADD)

fuzz_nxcp_receiver_SOURCES = fuzz-nxcp-receiver.cpp
fuzz_nxcp_receiver_CPPFLAGS = $(FUZZ_CPPFLAGS)
fuzz_nxcp_receiver_LDFLAGS = $(FUZZ_LDFLAGS)
fuzz_nxcp_receiver_LDADD = $(FUZZ_LDADD)

# Seed generator is an ordinary program, libFuzzer is not linked into it
fuzz_nxcp_seedgen_SOURCES = fuzz-nxcp-seedgen.cpp
fuzz_nxcp_seedgen_CPPFLAGS = $(FUZZ_CPPFLAGS)
fuzz_nxcp_seedgen_LDFLAGS = -no-install
fuzz_nxcp_seedgen_LDADD = $(FUZZ_LDADD)

noinst_HEADERS = fuzz-nxcp.h

EXTRA_DIST = corpus Makefile.am README.md
