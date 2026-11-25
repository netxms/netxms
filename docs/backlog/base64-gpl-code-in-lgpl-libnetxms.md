---
worth: yes
where: src/libnetxms/base64.cpp, include/base64.h
added: 2026-11-25
---
# base64 implementation in libnetxms is under GPL while the library is LGPL

`src/libnetxms/base64.cpp` and `include/base64.h` are the gnulib base64 module (Simon Josefsson,
Free Software Foundation) and carry a "GPL version 2 or later" header. `libnetxms` as a whole is
distributed under LGPL, so these two files are not compatible with the library's license. Replace
them with an implementation under LGPL or a permissive license, keeping the `base64_encode` /
`base64_decode` family of functions that the rest of the tree calls.
