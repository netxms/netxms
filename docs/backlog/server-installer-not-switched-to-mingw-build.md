---
worth: yes
where: src/install/windows/netxms-server-x64.iss:265
added: 2026-09-29
---
# server installer still packages the MSVC-era third-party DLL set

Master builds on Windows only with llvm-mingw, and the agent installers were switched to it in e37654ea07.
`netxms-server-x64.iss` was not: it ships the old DLL names, which do not match the MinGW SDK
(`C:/SDK-llvm-mingw`) that the binaries link against. Examples:

| server installer | MinGW SDK / agent installer |
|---|---|
| `libmicrohttpd.dll` | `libmicrohttpd-12.dll` |
| `pcre.dll`, `pcre16.dll` | `libpcre-1.dll`, `libpcre16-0.dll` |
| `libexpat.dll` | `libexpat-1.dll` |
| `zlib.dll` | `zlib1.dll` |
| `modbus.dll` | `libnxmodbus-5.dll` |
| `jq.dll` | `libjq-1.dll`, `libonig-5.dll` |
| `openssl-3\*` subdirectory | top-level `libssl-3-x64.dll`, `libcrypto-3-x64.dll`, `capi.dll` |

The MinGW runtime DLLs (`libc++.dll`, `libunwind.dll`, `libwinpthread-1.dll`) are missing too, and the
OpenSSL 1.1 DLLs are presumably dead. A server installed from a MinGW build this way would fail to load
`nxcore.dll` at startup. Surfaced while adding `libmicrohttpd` to the test suite DLL path in `Makefile.w32`.

Fix follows the agent conversion: mirror e37654ea07 for the server script, and add the DB client DLLs
(`libpq`, `libmariadb`, `libmysql`) and `libstrophe` from their MinGW SDK locations.
