---
worth: yes
where: src/libnetxms/tools.cpp:3019
added: 2026-10-24
---
# DecryptPasswordW leaks two strings when base64 decoding yields the wrong size

`DecryptPasswordW` allocates `mbencrypted` and `mblogin` with `MBStringFromWideStringSysLocale`,
then returns through `DecryptPasswordFailW` when `encSize` comes out wrong without freeing either.
A 44- or 88-character value made of base64 characters that does not decode to 32/64 bytes leaks
both strings on every call. Fix: free them before the failure return, or allocate `mblogin` only
after the size check.

Found while fixing issue #3827 (MQTT subagent password decoding).
