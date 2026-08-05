# nxai - NetXMS AI Assistant client

Command line client for NetXMS AI assistant. Communicates with the server via web API,
so it does not require direct access to the NetXMS server port.

## Building

The tool is built together with other client components:

```bash
./configure --prefix=/opt/netxms --with-client
make
make install
```

Line editing, command history, and command completion in interactive mode require libedit
(development package `libedit-dev` on Debian/Ubuntu, `libedit-devel` on RHEL/Fedora). If it
is not available, the tool is built with simple line input instead.

## Quick start

```bash
# Connect to a server (will prompt for credentials)
nxai -s netxms.example.com

# Connect with user name
nxai -s netxms.example.com -u admin

# Use environment variables
export NETXMS_SERVER=netxms.example.com
export NETXMS_USER=admin
export NETXMS_PASSWORD=secret
nxai
```

## Usage

```
Usage: nxai [OPTIONS] [message]

If message is given on command line or provided on standard input, it is sent to assistant
and tool exits after printing response. Otherwise interactive session is started.

Options:
  -h, --help                Display this help message.
  -i, --incident <id>       Set incident with given ID as conversation context.
  -n, --node <name>         Set node with given name as conversation context.
  -o, --object <id>         Set object with given ID as conversation context.
  -p, --password <password> Password for authentication.
  -s, --server <server>     Server host name or URL (for example netxms.local or
                            https://netxms.local:8443).
  -u, --user <user>         User name for authentication.
  -V, --version             Display version information.
      --clear-session       Delete saved session for server and exit.
      --no-save-session     Do not save session token for reuse.
      --no-verify-ssl       Do not verify server SSL certificate.
      --plain               Force plain text output without colors and formatting.

Environment variables NETXMS_SERVER, NETXMS_USER, and NETXMS_PASSWORD are used as
defaults for options -s, -u, and -p.
```

## Non-interactive use

If a message is given on the command line or piped to standard input, the tool sends it,
prints the response, and exits. Output is written as plain text without colors when it is
redirected, so it can be processed by other tools.

```bash
nxai -n web-server-01 "why is CPU high on this node?"

echo "summarize alarms for last hour" | nxai > report.txt
```

Questions asked by the assistant cannot be answered when standard input is not a terminal.
In that case they are automatically declined and a warning is printed.

## Interactive session

```
$ nxai -s netxms.local -n web-server-01
Using saved session for https://netxms.local
Conversation context set to web-server-01 [123]

NetXMS AI Assistant
Connected to https://netxms.local
Type your questions or commands. Use /help for list of available commands.

You> Why is CPU high on this node?
- Executing get_dci_values...

CPU utilization on web-server-01 has been above 90% for the last 15 minutes.
Top processes are:

  Process    CPU %
  java       45.2
  mysqld     32.1

Would you like me to investigate the Java process further?

You> yes
```

## Slash commands

| Command | Description |
|---------|-------------|
| `/help` | Show available commands |
| `/quit`, `/exit`, `/q` | Exit the chat |
| `/clear` | Clear chat history |
| `/object <name>` | Set object context (clears context if used without argument) |
| `/incident <id>` | Set incident context (clears context if used without argument) |
| `/status` | Show current session information |

## Keyboard shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+C` | Cancel current request or input |
| `Ctrl+D` | Exit the chat |
| `Up`/`Down` | Navigate command history |
| `Tab` | Complete slash command |

## Configuration

Access token received from the server is stored in `~/.config/nxai/sessions.json` (in
`%APPDATA%\nxai` on Windows) and reused by subsequent runs, so credentials have to be
provided only once. File is created with access rights allowing access only for current
user. Session can be deleted with

```bash
nxai -s netxms.example.com --clear-session
```

Command history is stored in `~/.config/nxai/history`.

## Conversation context

Chat can be bound to a NetXMS object or incident, so that assistant has context for the
conversation:

```bash
# By node name
nxai -s netxms.local -n web-server-01

# By object ID
nxai -s netxms.local -o 123

# By incident ID
nxai -s netxms.local -i 456
```

Context can also be changed during interactive session:

```
You> /object database-server
Conversation context set to database-server [789]

You> What's the status of this server?
```

## Troubleshooting

### SSL certificate errors

If server uses self-signed certificate:

```bash
nxai -s netxms.local --no-verify-ssl
```

### Connection issues

1. Verify that server is running and web API is enabled
2. Check that firewall rules allow access to web API port
3. Try with explicit protocol and port: `nxai -s https://netxms.local:8443`

### Authentication fails

1. Verify that user name and password are correct
2. Check if user is allowed to access web API
3. Try to delete saved session: `nxai -s netxms.local --clear-session`

## License

GPL-2.0-or-later

Copyright (C) 2025-2026 Raden Solutions
