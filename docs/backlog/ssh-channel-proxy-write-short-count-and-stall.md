---
worth: yes
where: src/agent/subagents/ssh/channel_proxy.cpp
added: 2026-11-05
---
# SSH channel proxy write drops short writes and can stall all proxies

`SSHChannelProxy::writeToChannel` ignores a positive short count from `ssh_channel_write` (blocking session
hitting its timeout while waiting for the remote window), so the rest of the keystroke/paste data is lost
silently.

The write runs on the agent comm session reader thread (`CMD_SSH_CHANNEL_DATA` is a raw message handled
inline) while `HandleSSHChannelCommand` holds the global `s_channelProxyLock`, so one channel waiting for
window adjust stalls every channel proxy and the whole agent session's inbound traffic. The write also holds
the per-proxy `m_sshLock`, so the reader cannot drain output meanwhile; a large paste into an echoing command
may hit flow-control stall (remote blocked on its output window while we wait for its input window) until
the session timeout. Untested — try pasting a few hundred KB into `cat` through the proxy terminal.

Surfaced while fixing the libssh 0.11 `SSH_AGAIN` disconnect and adding `m_sshLock`; the behaviour predates
that change. Fix direction: queue writes per proxy and let the reader thread interleave them with
non-blocking reads (session in non-blocking mode), retrying the remainder of short writes.
