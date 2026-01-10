# AI CLI Chat Client Design

## Overview

A command-line interface tool (`nxai`) for interactive terminal-based chat with the NetXMS AI assistant (Iris). The solution consists of two components:

1. **Server-side REST API** - New WebAPI endpoints exposing AI chat functionality
2. **Python CLI tool** - Interactive terminal client using the REST API

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  nxai (Python CLI)                                      │
│  ┌─────────────┐ ┌─────────────┐ ┌──────────────────┐  │
│  │ Chat REPL   │ │ Rich Output │ │ Session Manager  │  │
│  │ (prompt_    │ │ (rich lib)  │ │ (token storage)  │  │
│  │  toolkit)   │ │             │ │                  │  │
│  └─────────────┘ └─────────────┘ └──────────────────┘  │
└────────────────────────┬────────────────────────────────┘
                         │ HTTP/REST
                         ▼
┌─────────────────────────────────────────────────────────┐
│  NetXMS Server (webapi)                                 │
│  ┌──────────────────────────────────────────────────┐  │
│  │  /api/v1/ai/*  endpoints                          │  │
│  │  - POST /chat          (create chat session)      │  │
│  │  - POST /chat/{id}/message  (send message)        │  │
│  │  - GET  /chat/{id}/question (poll for questions)  │  │
│  │  - POST /chat/{id}/answer   (answer question)     │  │
│  │  - DELETE /chat/{id}        (end session)         │  │
│  └──────────────────────────────────────────────────┘  │
│                            │                            │
│                            ▼                            │
│  ┌──────────────────────────────────────────────────┐  │
│  │  IRIS Core (src/server/core/iris.cpp)             │  │
│  │  - CreateAIAssistantChat()                        │  │
│  │  - GetAIAssistantChat()                           │  │
│  │  - Chat::sendRequest()                            │  │
│  │  - Chat::getPendingQuestion()                     │  │
│  │  - Chat::handleQuestionResponse()                 │  │
│  │  - DeleteAIAssistantChat()                        │  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

## Design Decisions

| Aspect | Decision | Rationale |
|--------|----------|-----------|
| Interaction mode | Interactive REPL | Primary use case is conversational troubleshooting |
| Context binding | Object + Incident | Flexibility for different workflows |
| Authentication | CLI args, env vars, saved token | Convenient for both interactive and scripted use |
| Terminal output | Adaptive rich/plain | Best experience on capable terminals, works everywhere |
| Question handling | Polling-based | Simple for v1, avoids WebSocket complexity |
| Language | Python | Excellent terminal UI libraries, quick iteration |

## REST API Specification

**Base path:** `/api/v1/ai`

**Authentication:** Standard WebAPI auth (session token in `Authorization` header)

### POST `/chat`

Create new chat session.

**Request:**
```json
{
  "incidentId": 12345,
  "objectId": 67890
}
```

Both fields are optional. Use `incidentId` to bind chat to an incident, `objectId` to bind to a NetXMS object.

**Response:**
```json
{
  "chatId": 1001,
  "created": "2026-01-09T10:30:00Z"
}
```

### POST `/chat/{chatId}/message`

Send message to AI assistant.

**Request:**
```json
{
  "message": "Why is server-01 showing high CPU?",
  "context": {
    "objectId": 67890
  }
}
```

The `context` field is optional and used to update context mid-session.

**Response:**
```json
{
  "response": "Looking at server-01, I can see...",
  "pendingQuestion": null
}
```

If the AI needs user input, `pendingQuestion` will contain the question object (see GET `/chat/{id}/question` response format).

### GET `/chat/{chatId}/question`

Poll for pending question from AI.

**Response (no question):**
```json
{
  "question": null
}
```

**Response (question pending):**
```json
{
  "question": {
    "id": 5001,
    "type": "confirmation",
    "confirmationType": "approve_reject",
    "text": "Execute this command?",
    "context": "systemctl restart nginx",
    "options": [],
    "expiresAt": "2026-01-09T10:35:00Z"
  }
}
```

| Field | Description |
|-------|-------------|
| `type` | `confirmation` or `multipleChoice` |
| `confirmationType` | For confirmations: `approve_reject`, `yes_no`, or `confirm_cancel` |
| `options` | Array of strings for `multipleChoice` type |
| `context` | Additional context (e.g., command to be executed) |
| `expiresAt` | ISO 8601 timestamp when question expires |

### POST `/chat/{chatId}/answer`

Answer a pending question.

**Request:**
```json
{
  "questionId": 5001,
  "positive": true,
  "selectedOption": -1
}
```

| Field | Description |
|-------|-------------|
| `questionId` | ID of the question being answered |
| `positive` | For confirmations: `true` = approve/yes/confirm |
| `selectedOption` | For multipleChoice: 0-based index, -1 for confirmations |

**Response:**
```json
{
  "success": true
}
```

### DELETE `/chat/{chatId}`

End and delete chat session.

**Response:**
```json
{
  "success": true
}
```

### Error Responses

All errors return appropriate HTTP status with JSON body:

```json
{
  "error": "Chat session not found",
  "code": "CHAT_NOT_FOUND"
}
```

| Scenario | HTTP Status | Code |
|----------|-------------|------|
| Invalid credentials | 401 | `AUTH_FAILED` |
| Chat not found | 404 | `CHAT_NOT_FOUND` |
| Chat owned by another user | 403 | `ACCESS_DENIED` |
| Question expired/already answered | 409 | `QUESTION_INVALID` |
| AI backend unavailable | 503 | `AI_UNAVAILABLE` |
| Request timeout (LLM slow) | 504 | `TIMEOUT` |

## Python CLI Tool

### Installation

Location: `src/client/nxai/`

```bash
cd src/client/nxai
pip install .

# Or install in development mode
pip install -e .
```

### Dependencies

| Package | Purpose |
|---------|---------|
| `rich` | Terminal formatting, markdown rendering, syntax highlighting |
| `prompt_toolkit` | Interactive input, history, auto-completion |
| `httpx` | Modern HTTP client |
| `keyring` (optional) | Secure token storage |

### Command-line Interface

```bash
# Basic usage - interactive mode
nxai

# With connection options
nxai --server netxms.example.com --user admin

# With context binding
nxai --node server-01
nxai --incident 12345
nxai --object 67890          # by object ID

# Environment variables (fallback)
export NETXMS_SERVER=netxms.example.com
export NETXMS_USER=admin
export NETXMS_PASSWORD=secret  # or prompt if not set
```

### Session Token Storage

After successful login, save encrypted token to `~/.config/nxai/session.json`:

```json
{
  "server": "netxms.example.com",
  "token": "...",
  "expires": "2026-01-10T10:30:00Z"
}
```

On subsequent runs, attempt to reuse token. If expired or invalid, prompt for credentials.

### Interactive Session Example

```
$ nxai --server netxms.local --node web-server-01
Connecting to netxms.local...
Password: ********
Connected as admin. Context: web-server-01 (Node)

Type /help for commands, /quit to exit.

You> Why is CPU high on this node?

Iris> Looking at web-server-01, I can see CPU utilization has been
      above 90% for the last 15 minutes. The top processes are:

      | Process    | CPU % |
      |------------|-------|
      | java       | 45.2  |
      | mysqld     | 32.1  |

      Would you like me to investigate the Java process further?

You> yes

Iris> ...
```

### Slash Commands

| Command | Description |
|---------|-------------|
| `/help` | Show available commands |
| `/quit` or `/exit` | End session |
| `/clear` | Clear chat history |
| `/object <name>` | Switch object context |
| `/incident <id>` | Switch to incident context |
| `/status` | Show current session info |

### Module Structure

```
src/client/nxai/
├── pyproject.toml           # Package metadata, dependencies
├── README.md                # Usage documentation
└── nxai/
    ├── __init__.py
    ├── __main__.py          # Entry point (python -m nxai)
    ├── cli.py               # Argument parsing, main loop
    ├── client.py            # REST API client
    ├── session.py           # Token storage, auth handling
    ├── chat.py              # Chat REPL logic
    ├── render.py            # Output formatting (rich/plain)
    ├── questions.py         # Question polling & answering
    └── commands.py          # Slash commands (/help, /object, etc.)
```

### Key Classes

```python
# client.py
class NetXMSClient:
    """REST API client for NetXMS WebAPI"""
    def __init__(self, server: str, token: str): ...
    def create_chat(self, incident_id=None, object_id=None) -> int: ...
    def send_message(self, chat_id: int, message: str, context=None) -> ChatResponse: ...
    def poll_question(self, chat_id: int) -> Question | None: ...
    def answer_question(self, chat_id: int, question_id: int, positive: bool, option: int): ...
    def delete_chat(self, chat_id: int): ...

# chat.py
class ChatSession:
    """Manages interactive chat session"""
    def __init__(self, client: NetXMSClient, renderer: Renderer): ...
    def run(self):  # Main REPL loop
    def handle_input(self, text: str): ...
    def check_pending_questions(self): ...

# render.py
class Renderer:
    """Adaptive output rendering"""
    def __init__(self, force_plain: bool = False): ...
    def render_response(self, text: str): ...      # Markdown rendering
    def render_question(self, question: Question): ...
    def is_interactive(self) -> bool: ...          # TTY detection
```

## Server-side Implementation

### New File: `src/server/webapi/ai.cpp`

Follows existing WebAPI patterns.

```cpp
// POST /api/v1/ai/chat
static int H_CreateChat(Context *context)
{
   uint64_t incidentId = /* from JSON body */;
   uint32_t userId = context->getUserId();
   Chat *chat = CreateAIAssistantChat(userId, incidentId);
   // Return chat ID as JSON
}

// POST /api/v1/ai/chat/{id}/message
static int H_SendMessage(Context *context)
{
   uint32_t chatId = /* from path */;
   Chat *chat = GetAIAssistantChat(chatId);
   // Verify ownership
   String response = chat->sendRequest(message, contextJson);
   // Return response + pending question state
}

// GET /api/v1/ai/chat/{id}/question
static int H_PollQuestion(Context *context)
{
   Chat *chat = GetAIAssistantChat(chatId);
   PendingQuestion *q = chat->getPendingQuestion();
   // Return question or null
}

// POST /api/v1/ai/chat/{id}/answer
static int H_AnswerQuestion(Context *context)
{
   Chat *chat = GetAIAssistantChat(chatId);
   chat->handleQuestionResponse(questionId, positive, selectedOption);
}

// DELETE /api/v1/ai/chat/{id}
static int H_DeleteChat(Context *context)
{
   DeleteAIAssistantChat(chatId);
}
```

### IRIS Core Additions

Add non-blocking method to `Chat` class for question polling:

```cpp
// src/server/include/iris.h
class Chat
{
   // ... existing members ...

   // Non-blocking check for pending question (for WebAPI polling)
   PendingQuestion* getPendingQuestion() const;
};
```

### Route Registration

In `src/server/webapi/main.cpp`:

```cpp
void RegisterAiHandlers(Router *router);
```

## Error Handling

### CLI Error Handling

| Scenario | Behavior |
|----------|----------|
| Connection lost | Attempt reconnect, show status, allow retry |
| Session token expired | Prompt for re-authentication |
| Question timeout | Show "Question expired" message, continue chat |
| Ctrl+C during response | Cancel gracefully, stay in session |
| Ctrl+D or `/quit` | Clean exit, delete chat session |

### Timeout Considerations

LLM responses can take 30+ seconds.

**REST API:**
- Use long timeout for `/chat/{id}/message` endpoint (120s recommended)
- Return appropriate 504 status on timeout

**CLI:**
- Show spinner/progress indicator while waiting
- Allow Ctrl+C to cancel pending request

## Future Enhancements (v2+)

- **Streaming responses** - WebSocket or SSE for real-time token streaming
- **One-shot mode** - `nxai -q "Why is server down?"` for scripting
- **JSON output mode** - `nxai --json` for programmatic use
- **Shell completion** - Tab completion for object names, commands
- **Chat history** - Persistent local history across sessions

## Implementation Checklist

### Server-side (WebAPI)

- [ ] Create `src/server/webapi/ai.cpp`
- [ ] Implement `H_CreateChat` handler
- [ ] Implement `H_SendMessage` handler
- [ ] Implement `H_PollQuestion` handler
- [ ] Implement `H_AnswerQuestion` handler
- [ ] Implement `H_DeleteChat` handler
- [ ] Add `Chat::getPendingQuestion()` method to IRIS core
- [ ] Register routes in `main.cpp`
- [ ] Add to build system (`Makefile.am`)

### Client-side (Python CLI)

- [ ] Create `src/client/nxai/` directory structure
- [ ] Create `pyproject.toml` with dependencies
- [ ] Implement `client.py` - REST API client
- [ ] Implement `session.py` - Token storage and auth
- [ ] Implement `render.py` - Adaptive terminal output
- [ ] Implement `questions.py` - Question handling
- [ ] Implement `commands.py` - Slash commands
- [ ] Implement `chat.py` - Main REPL logic
- [ ] Implement `cli.py` - Argument parsing and entry point
- [ ] Create `README.md` with usage documentation
