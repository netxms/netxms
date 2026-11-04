---
worth: yes
where: src/agent/subagents/ssh/channel_proxy.cpp
added: 2026-11-05
---
# SSH channel proxies outlive the comm session that opened them

`SSHChannelProxy` is only deleted on `CMD_CLOSE_SSH_CHANNEL` from the server, when the remote side closes the
channel, or at subagent shutdown. When the owning agent comm session goes away without a close request (session
watchdog, server crash, network loss), the proxy keeps its reader thread, the libssh channel, the dedicated
`SSHSession` and a reference to the dead `CommSession`, posting output nowhere. With a shell that has no idle
logout this lasts until agent restart.

Seen in the agent log while diagnosing issue #3876: the session carrying channel 4 was dropped by
"Session disconnected by watchdog" and nothing logged the proxy being deleted. The subagent API has no session-close
notification (`AGENT_NOTIFY_*` covers only policy install and token receipt), so the fix needs either such a hook or
the reader thread checking the session's state; `AbstractCommSession` exposes neither today.
