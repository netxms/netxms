# nxshell - NetXMS shell

Command line shell for NetXMS server. It combines object navigation, server debug console,
AI assistant, and ad-hoc NXSL script execution under one prompt, and can be extended with
user defined commands written in NXSL. The shell communicates with the server via web API,
so it does not require direct access to the NetXMS server port.

The tool is also installed under the name `nxai`. When started as `nxai` it works as AI
assistant client (see [AI assistant client](#ai-assistant-client-nxai)).

## Building

The tool is built together with other client components:

```bash
./configure --prefix=/opt/netxms --with-client
make
make install
```

Line editing, command history, and completion in interactive mode require libedit
(development package `libedit-dev` on Debian/Ubuntu, `libedit-devel` on RHEL/Fedora). If it
is not available, the tool is built with simple line input instead.

## Quick start

```bash
# Connect to a server (will prompt for credentials)
nxshell -s netxms.example.com

# Connect with user name
nxshell -s netxms.example.com -u admin

# Use environment variables
export NETXMS_SERVER=netxms.example.com
export NETXMS_USER=admin
export NETXMS_PASSWORD=secret
nxshell
```

## Usage

```
Usage: nxshell [OPTIONS] [file]

Commands are read from given file, or from standard input if it is not a terminal, and
executed until first failure. Otherwise interactive session is started.

Options:
  -c, --command <line>      Execute given line and exit (can be used multiple times).
  -h, --help                Display this help message.
  -i, --incident <id>       Set incident with given ID as AI assistant conversation context.
  -n, --node <name>         Set object with given name as current object.
  -o, --object <id>         Set object with given ID as current object.
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

## Commands

Input line is a command: either a builtin command or an alias. Unknown commands are
reported as errors and are never sent to the server.

| Command | Description |
|---------|-------------|
| `cd [path \| #id \| @name \| -]` | Change current object |
| `ls [path]` | List child objects |
| `pwd` | Show path to current object |
| `ai [message]` | Send message to AI assistant or switch to AI assistant mode |
| `console [command]`, `con [command]` | Execute server debug console command or switch to console mode |
| `nxsl [code]` | Execute NXSL code or switch to NXSL mode |
| `= <expression>` | Evaluate NXSL expression and print its value |
| `alias [name [= code \| { code }]]` | List, show, or define aliases |
| `unalias <name>` | Remove alias |
| `status` | Show current session information |
| `help` | Show list of commands |
| `exit`, `quit` | Leave current mode or exit the shell |

Lines starting with `#` are comments.

### Modes

Commands `ai`, `console`, and `nxsl` followed by text execute that text once:

```
netxms.local:/> console show pollers
netxms.local:/> ai why is router1 unreachable?
netxms.local:/> nxsl println(GetServerNodeId());
```

Used without arguments they switch the shell to corresponding mode, where every line is
sent to that target. Current mode is shown in the prompt. A line that starts with `/`
followed by a command is executed as shell command, so navigation and other targets stay
available. Command `/exit` or `Ctrl+D` returns to shell mode.

```
netxms.local:/> console
netxms.local:/ console> show flags
netxms.local:/ console> /cd Infrastructure Services
netxms.local:/Infrastructure Services console> /exit
netxms.local:/Infrastructure Services>
```

AI assistant mode has two additional commands: `/clear` clears chat history, and
`/incident <id>` sets an incident as conversation context (`/incident` without argument
clears it).

## Current object

The shell has a current object, selected by navigating object tree. It is shown in the
prompt, NXSL code and aliases are executed in its context (`$object`, and `$node` if it is
a node), and it is the conversation context for AI assistant. At the top level (`/`) there
is no current object.

```
netxms.local:/> ls
       1  Network      Normal     Entire Network
       2  ServiceRoot  Normal     Infrastructure Services
netxms.local:/> cd Infrastructure Services/Core/router1
netxms.local:/Infrastructure Services/Core/router1> cd ..
netxms.local:/Infrastructure Services/Core> cd /
```

* Path elements are object names, matched ignoring case. Path that starts with `/` is
  absolute, otherwise it is relative to current object. `..` is parent object in the path.
* Spaces in names do not require quoting. Character `/` within a name is written as `\/`
  (for example `cd Gi0\/1`), and `\` as `\\`.
* `cd -` returns to previous location, `cd` without arguments goes to the top level.
* `cd #<id>` selects object by ID and `cd @<name>` by name search; if multiple objects
  match the name, the shell asks which one to select. Path to such object is built by
  following its first accessible parent.
* If multiple child objects have the same name, use `cd #<id>` to select one of them.
* `Tab` completes command names and object names in arguments of `cd` and `ls`.

## NXSL

NXSL code is executed on the server in context of current object. Script output is
displayed as it is produced. If script returns a value other than null, it is printed
after the output; arrays and hash maps are printed as JSON.

```
netxms.local:/Infrastructure Services/Core/router1> = $node.ipAddr
10.0.0.1
netxms.local:/Infrastructure Services/Core/router1> nxsl
netxms.local:/Infrastructure Services/Core/router1 nxsl> for(i : $node.interfaces) {
...>    println(i.name);
...> }
```

Input continues on the next line until all brackets are closed. Every input is executed
as separate script, so variables are not kept between inputs. Running script can be
stopped with `Ctrl+C`.

Script execution for an object requires "execute script" access right on that object.
Execution without current object requires system access right to manage scripts.

## Aliases

Alias is a command defined as NXSL code. It is executed in context of current object, and
command arguments are passed as script parameters (`$1`, `$2`, ..., or `$ARGS`).

```
alias up = return $node.status == 0;

alias uptime {
   println($node.name .. " is up since " .. DateTime($node.bootTime).format("%Y-%m-%d %H:%M"));
}
```

`alias` without arguments lists all aliases, and `alias <name>` shows alias definition.
Default aliases `info`, `alarms`, `dci`, and `interfaces` are always available and can be
replaced by user defined aliases with the same name. Builtin commands cannot be replaced.

Aliases defined at the prompt are kept until the shell exits. To make them permanent, put
them into startup file.

### Startup file

File `~/.config/nxshell/nxshellrc` (`%APPDATA%\nxshell\nxshellrc` on Windows) is executed
every time the shell starts, using the same syntax as interactive input. Failed commands
are reported and do not prevent the shell from starting.

## Non-interactive use

Commands can be given with `-c` option, in a file, or on standard input. Execution stops
at first failed command. Output is written as plain text without colors when it is
redirected, and status messages are written to standard error stream, so output can be
processed by other tools.

```bash
nxshell -c 'cd /Infrastructure Services/Core/router1' -c 'alarms'

nxshell -n router1 -c '= $node.interfaces.size'

nxshell maintenance.nxsh

echo 'console show queues' | nxshell > queues.txt
```

| Exit code | Meaning |
|-----------|---------|
| 0 | All commands completed successfully |
| 1 | Command failed (unknown command, object not found, script error, access denied) |
| 2 | Cannot connect to the server or authentication failed |

Server debug console does not report command failures, so console commands are always
considered successful if they were delivered to the server.

## Keyboard shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+C` | Cancel current operation or input |
| `Ctrl+D` | Leave current mode or exit the shell |
| `Up`/`Down` | Navigate command history |
| `Tab` | Complete command or object name |

## AI assistant client (nxai)

When the tool is started as `nxai`, it starts in AI assistant mode, and leaving that mode
exits the tool. Message given on command line or provided on standard input is sent to
the assistant, and the tool exits after printing response:

```bash
nxai -n web-server-01 "why is CPU high on this node?"

echo "summarize alarms for last hour" | nxai > report.txt
```

Questions asked by the assistant cannot be answered when standard input is not a terminal.
In that case they are automatically declined and a warning is printed.

Conversation context is current object or, if set with `-i` option or `/incident`
command, an incident. Shell commands are available with `/` prefix, for example `/cd` to
change current object or `/console show pollers`.

## Configuration

Access token received from the server is stored in `~/.config/nxshell/sessions.json` (in
`%APPDATA%\nxshell` on Windows) and reused by subsequent runs, so credentials have to be
provided only once. File is created with access rights allowing access only for current
user. If the server does not accept saved token, the shell authenticates again. Saved
session can be deleted with

```bash
nxshell -s netxms.example.com --clear-session
```

Command history is stored in `~/.config/nxshell/history`.

## Troubleshooting

### SSL certificate errors

If server uses self-signed certificate:

```bash
nxshell -s netxms.local --no-verify-ssl
```

### Connection issues

1. Verify that server is running and web API is enabled
2. Check that firewall rules allow access to web API port
3. Try with explicit protocol and port: `nxshell -s https://netxms.local:8443`

### Authentication fails

1. Verify that user name and password are correct
2. Check if user is allowed to access web API
3. Try to delete saved session: `nxshell -s netxms.local --clear-session`

## License

GPL-2.0-or-later

Copyright (C) 2025-2026 Raden Solutions
