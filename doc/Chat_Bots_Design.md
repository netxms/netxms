# Chat Bots — Interactive Chat Interface Design Document

Status: design draft (no issue yet)

This document describes a new server entity — **chat bot** — providing
bidirectional, interactive access to the server over messaging platforms
(Telegram, Mattermost, XMPP, later Slack), primarily as a front end to the
AI assistant. Notification channels remain one-way; a chat bot owns the
platform connection and additionally provides a notification channel
automatically.

## 1. Background and motivation

Notification channels deliver rendered one-way messages. For chat platforms
(Telegram, Slack, Mattermost, XMPP) users increasingly expect to talk back:
ask about alarms, node status, or run actions from the same bot that sends
alerts. The server already has the machinery for the hard part — the AI
assistant (`nxai.h`):

- `Chat` is a stateful, user-scoped conversation (`m_userId` scopes every
  assistant function through the user's ACLs).
- `askConfirmation()` / `askMultipleChoice()` with `PendingQuestion` map
  directly onto platform-native interactive elements (Telegram inline
  keyboards).

Design decisions locked during brainstorming:

1. **AI assistant access is the v1 capability.** A structured command system
   and debug console access are deferred; they slot in later as alternative
   message handlers in front of the AI dispatch.
2. **Identity via admin-defined mapping**: platform user ID → NetXMS login,
   stored per bot. No self-service binding in v1.
3. **A chat bot doubles as a notification channel** (auto-registered),
   because both roles want the same bot identity and the transports conflict
   physically: Telegram allows a single `getUpdates` consumer per bot token,
   so independent NC channel + chat bot on one token would steal each
   other's updates.
4. **Shared driver housing, separate interfaces**: `NCDriver` stays strictly
   one-way; the bidirectional role is a separate abstract class provided by
   the same scan-loaded driver binary via an additional optional entry
   point. No dilution of the notification concept, no duplicated platform
   plumbing.
5. **One conversation per mapped user per bot**, idle expiry, `/new` reset.
6. **Direct messages only in v1**; group chats deferred.
7. **v1 platforms: Telegram, Mattermost, XMPP.** Slack deferred (needs
   Socket Mode WebSocket; enabled by the Mattermost WS client work).

## 2. Entity model

A **ChatBot** is a named, admin-created object mirroring notification
channels structurally:

- name, description;
- driver name (`telegram`, `mattermost`, `xmpp`);
- driver-specific configuration (`Config` text, as NC channels);
- user mapping list: platform peer ID → NetXMS user ID;
- session settings: idle timeout, AI provider slot.

Core keeps a registry analogous to the NC channel registry in
`notification_channel.cpp`: load at startup, CRUD via NXCP and WebAPI,
start/stop of driver instances on configuration changes.

### 2.1 Automatic notification channel

Creating a chat bot registers a notification channel of the same name. The
channel is backed by a core-side `NCDriver` adapter wrapping the bot's live
`ChatBotDriver` instance: it receives the full v5 `NotificationContext` and
forwards `recipient`/`subject`/`body` through `sendMessage()`. One
connection, one token — no transport conflict. Because the adapter is a
real v5 driver, rich formatting (rendering `context.event` severity/source
natively) is a later driver-internal improvement with no interface change.

In the channel list this entry is flagged "provided by chat bot X" and is
not independently editable or deletable; it disappears with the bot.
Standalone telegram/mattermost/xmpp NC channels keep working unchanged for
notification-only use — but a standalone channel configured with the *same*
token as a chat bot must be deleted (documented; the bot provides the
channel).

Naming note: "ChatInterface" was rejected because *Interface* is a core
object class; "ChatBot" is what the admin actually configures.

## 3. Driver layer

`NCDriver` is untouched. A new header `src/server/include/chatdrv.h`
defines the bidirectional role (all strings UTF-8, consistent with NC API
v5):

```cpp
class ChatBotMessageSink   // implemented by core, handed to the driver
{
public:
   virtual void onMessage(const char *peerId, const char *displayName, const char *text) = 0;
   virtual void onChoiceResponse(const char *peerId, uint64_t questionId, int selectedOption) = 0;
};

class ChatBotDriver
{
public:
   virtual bool start(ChatBotMessageSink *sink) = 0;   // driver owns its transport thread(s)
   virtual void stop() = 0;
   virtual bool sendMessage(const char *peerId, const char *text) = 0;
   virtual bool sendQuestion(const char *peerId, const char *text, const StringList& options, uint64_t questionId) = 0;
   virtual bool checkHealth() { return true; }
};
```

### 3.1 Entry point

No `NCDRV_API_VERSION` bump. The loader (`LoadDriver` in
`notification_channel.cpp`) resolves exported symbols individually, so the
chat capability is one additional **optional** symbol:

```cpp
DECLARE_CHATBOT_ENTRY_POINT   // exports NcdCreateChatBotInstance(Config*, NCDriverStorageManager*)
```

declared alongside the existing `DECLARE_NCD_ENTRY_POINT`. Absent symbol =
pure notification driver; all existing drivers remain valid unmodified.
The chat factory reuses `NCDriverStorageManager` (the Telegram driver
already persists username→chat-ID mappings through it).

`RegisterChatBotDriver` (and its NC counterpart) are `NXCORE_EXPORTABLE`,
so modules can register drivers the same way `RegisterEventForwarderDriver`
is used today (issue #3382 precedent). The directory scan feeds both
registries through the same functions.

### 3.2 Question rendering fallback

If `sendQuestion()` returns `false` (platform has no interactive elements),
**core** renders the question as plain text with numbered options and
interprets the next numeric reply from that peer as the answer. Drivers
stay minimal; XMPP and v1 Mattermost use this path.

## 4. Core manager and message flow

Each `ChatBot` holds its driver instance, the resolved user mapping
(rebuilt on config change), and a session map:
peer ID → `{shared_ptr<Chat>, lastActivity, state}`.

**Inbound**: driver transport thread → `onMessage` → mapping lookup.
Unmapped peers are silently ignored (debug-logged, no reply — the bot does
not confirm its existence to strangers). For mapped users a built-in
command layer runs first: `/new` (reset conversation), `/help`, `/whoami`
(mapped identity summary). Everything else dispatches via
`ThreadPoolExecute` — never blocking the transport thread — to the
session's `Chat`, created on first message with
`CreateAIAssistantChat(userId, ...)` so every assistant function is
ACL-checked as that user. One outstanding request per session; messages
arriving mid-request are queued into the next prompt or answered with a
brief "still working". Replies go back through `sendMessage()`; the driver
translates or strips markdown to its platform dialect.

**Confirmations**: assistant functions call
`GetCurrentAIChat()->askConfirmation()`, blocking the worker on a
`Condition`. The nxmc client polls for pending questions; for chat bots,
`Chat` gains a small extension — an optional **question listener
callback** — so the session pushes `sendQuestion()` to the platform the
moment the question is posted. `onChoiceResponse` feeds
`handleQuestionResponse()` and the assistant resumes. Timeouts already
exist (`PendingQuestion::expiresAt`).

**Expiry**: a housekeeper sweep closes sessions idle past the configured
timeout and deletes the `Chat`.

## 5. Platform notes

### 5.1 Telegram (phase 1)

The driver already runs a `getUpdates` long-poll thread; the chat
capability extends it to dispatch `message` updates (only
`chat.type == "private"`) into the sink. Questions render as
`InlineKeyboardMarkup`; `callback_query` carries question ID + option index
and is acknowledged with `answerCallbackQuery`. Reply formatting uses HTML
parse mode (MarkdownV2 escaping is a known trap).

### 5.2 XMPP (phase 2)

libstrophe is natively bidirectional; the existing connection loop gains a
message-stanza handler. Peer identity is the bare JID. Numbered-list
question fallback.

### 5.3 Mattermost (phase 2)

Inbound requires a WebSocket client (`/api/v4/websocket`, token auth,
`posted` events). This is the one genuinely new piece of plumbing: no WS
client exists in-tree, and libcurl's WS API is only stable in recent 8.x
(above our floor), so the plan is a small RFC 6455 client over existing
TLS socket code. That client later enables Slack Socket Mode. Mattermost's
interactive buttons POST to an integration URL (inbound HTTP), which v1
avoids — numbered-list fallback instead.

### 5.4 Slack (deferred)

Socket Mode reuses the WS client; Block Kit buttons for questions.

## 6. Storage, protocol, and UI surface

### 6.1 Database (schema v70 migration + `sql/schema.in`)

- `chat_bots`: `name` (PK, varchar 64), `driver_name`, `description`,
  `configuration` (CLOB), plus core-managed columns `idle_timeout` and AI
  provider slot.
- `chat_bot_users`: `(channel_name, peer_id, user_id)`, PK
  `(channel_name, peer_id)` — explicit rows are auditable and edited in the
  GUI as a table with a user picker.

### 6.2 NXCP

Command set mirroring NC channels: get/create/update/delete/rename chat
bot, get status; same system access right as notification channel
configuration. New `CMD_*` codes registered in `NXCPMessageCodeName()` and
mirrored into `NXCPCodes.java`.

### 6.3 nxmc

"Chat Bots" view in the Configuration perspective next to Notification
Channels (SWT + RWT from the shared codebase). Editor: driver dropdown
(only drivers exporting the chat factory), driver configuration text,
mapping table, session settings. Status columns: driver health, connection
state, active session count, last inbound message time.

### 6.4 WebAPI

`v1/chat-bots` CRUD following the established JSON resource pattern.

### 6.5 Audit

Bot and mapping create/modify/delete audited like NC channel changes.
Session lifecycle events (session opened for user X from peer Y, question
answered) write audit records with the mapped NetXMS user as actor.

## 7. Security considerations

- Every AI action runs under the mapped NetXMS user's ACLs — the chat bot
  adds no privilege of its own.
- Unmapped senders receive no response whatsoever.
- DMs only in v1: sender identity is unambiguous, and confirmation buttons
  cannot be pressed by bystanders.
- Debug console access is explicitly out of scope for v1; when added, it
  requires its own access right gate in front of the message dispatch.

## 8. Phasing

1. **Framework + Telegram** — `chatdrv.h`, loader extension, `ChatBot`
   registry/session manager, user mapping, built-in commands, numbered-list
   fallback, `Chat` question listener, NC adapter channel, DB tables +
   v70 migration, NXCP + nxmc + WebAPI, Telegram chat factory with inline
   keyboards. One coherent deliverable; everything but the last item is
   platform-independent.
2. **XMPP, then Mattermost** — XMPP validates the numbered-list fallback
   cheaply; Mattermost carries the RFC 6455 WebSocket client investment.
3. **Deferred** — Slack (Socket Mode via the WS client); group chats
   (mention-triggered, sender-attributed buttons); self-service token
   binding for user mapping; structured command system and debug console
   access as additional gated message handlers.

Per the contribution workflow, a GitHub issue precedes implementation.
