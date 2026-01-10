# nxai - NetXMS AI Assistant CLI

Interactive terminal chat client for Iris, the NetXMS AI assistant.

## Installation

```bash
cd src/client/nxai
pip install .

# Or install in development mode
pip install -e .

# With secure token storage (optional)
pip install -e ".[secure]"
```

## Quick Start

```bash
# Connect to a server (will prompt for credentials)
nxai --server netxms.example.com

# Connect with username
nxai --server netxms.example.com --user admin

# Use environment variables
export NETXMS_SERVER=netxms.example.com
export NETXMS_USER=admin
export NETXMS_PASSWORD=secret
nxai
```

## Usage

```
usage: nxai [-h] [-s HOST] [-u USER] [-p PASS] [-n NAME | -o ID | -i ID]
            [--plain] [--no-verify-ssl] [--no-save-session] [--clear-session]
            [-V]

NetXMS AI Assistant CLI - Interactive chat with Iris

options:
  -h, --help            show this help message and exit
  -s HOST, --server HOST
                        NetXMS server hostname or URL
  -u USER, --user USER  Username for authentication
  -p PASS, --password PASS
                        Password (use environment variable instead for security)
  -n NAME, --node NAME  Node name for context
  -o ID, --object ID    Object ID for context
  -i ID, --incident ID  Incident ID for context
  --plain               Force plain text output (no colors or formatting)
  --no-verify-ssl       Disable SSL certificate verification
  --no-save-session     Don't save session token for reuse
  --clear-session       Clear saved session token and exit
  -V, --version         show program's version number and exit

Environment variables: NETXMS_SERVER, NETXMS_USER, NETXMS_PASSWORD
```

## Interactive Session

Once connected, you can chat with Iris:

```
$ nxai --server netxms.local --node web-server-01
Using saved session for netxms.local
Found: web-server-01 (ID: 123)
Starting chat session...

╭───────────────────────── Welcome ──────────────────────────╮
│ Iris - NetXMS AI Assistant                                 │
│                                                            │
│ Type your questions or commands. Use /help for available   │
│ commands.                                                  │
╰────────────────────────────────────────────────────────────╯

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

## Slash Commands

| Command | Description |
|---------|-------------|
| `/help` | Show available commands |
| `/quit`, `/exit`, `/q` | Exit the chat |
| `/clear` | Clear chat history |
| `/object <name>` | Set object context |
| `/incident <id>` | Set incident context |
| `/status` | Show current session info |

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+C` | Cancel current request |
| `Ctrl+D` | Exit the chat |
| `Up/Down` | Navigate command history |

## Configuration

Session tokens are stored in `~/.config/nxai/sessions.json` for automatic reconnection.

Command history is stored in `~/.config/nxai/history`.

To clear a saved session:
```bash
nxai --server netxms.example.com --clear-session
```

## Context Binding

You can bind the chat to a specific NetXMS object or incident for contextual awareness:

```bash
# By node name
nxai --server netxms.local --node web-server-01

# By object ID
nxai --server netxms.local --object 123

# By incident ID
nxai --server netxms.local --incident 456
```

You can also change context during a session:
```
You> /object database-server
Context set to: database-server (ID: 789)

You> What's the status of this server?
```

## Troubleshooting

### SSL Certificate Errors

If you're using a self-signed certificate:
```bash
nxai --server netxms.local --no-verify-ssl
```

### Connection Issues

1. Verify the server is running and WebAPI is enabled
2. Check firewall rules allow access to the WebAPI port
3. Try with explicit protocol: `nxai --server https://netxms.local:8443`

### Authentication Fails

1. Verify username and password are correct
2. Check if user has WebAPI access permissions
3. Try clearing saved session: `nxai --server HOST --clear-session`

## Requirements

- Python 3.9+
- NetXMS server with WebAPI enabled
- AI assistant (Iris) configured on the server

## License

GPL-2.0-or-later

Copyright (C) 2025-2026 Raden Solutions
