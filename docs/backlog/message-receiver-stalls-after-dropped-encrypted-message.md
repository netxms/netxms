---
worth: yes
where: src/libnetxms/msgrecv.cpp
added: 2026-11-17
---
# message receiver waits for more bytes after dropping an encrypted message

When `AbstractMessageReceiver::getMessageFromBuffer()` consumes a `CMD_ENCRYPTED_MESSAGE` and the receiver has
no encryption context, it returns neither a message nor an error. `readMessage()` then goes straight to
`readBytes()` instead of parsing the buffer again, so a complete message that arrived in the same read stays
in the buffer until more bytes come or the read times out.

Found while writing the receiver unit tests for the NXCP parser fuzzing work (`TestMessageReceiverBuffer` in
`tests/test-libnetxms/nxcp.cpp`); the "encrypted message without encryption context" case feeds a second block
only because of this. Not fixed there because it is a behaviour change outside that task. The fix is to parse
the buffer again whenever something was consumed without a result; the test can then drop its second block.

No user-visible symptom known: a peer has to send an encrypted message to a receiver without a context, and
the cost is a delayed message, not a lost one.
