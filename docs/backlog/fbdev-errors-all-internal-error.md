---
worth: yes
where: src/agent/subagents/fbdev/fbdev.cpp
added: 2026-11-20
---
# fbdev returns Internal error for every screenshot failure

Open, ioctl, unsupported color depth, mmap and PNG encoding failures all return `ERR_INTERNAL_ERROR`, so
a permissions problem on `/dev/fbN` looks the same as a driver problem; the cause is only in the `fbdev`
debug log. Map `EACCES` to `ERR_ACCESS_DENIED` and an unsupported depth to a distinct code. The admin guide
troubleshooting lists `500: Internal error` for all of them; update it when fixed.
