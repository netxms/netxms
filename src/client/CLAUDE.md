# Client Components

> Shared guidelines: See [root CLAUDE.md](../../CLAUDE.md) for Java development guidelines, build commands, and contribution workflow.

## Overview

The client components provide the user interface for NetXMS:
- **nxmc** - Management console (Java SWT/RWT)
- **netxms-client** - Java client library
- **libnxclient** - C++ client library
- Various command-line tools

## Directory Structure

```
src/client/
├── java/
│   └── netxms-client/    # Java client library
├── libnxclient/          # C++ client library
├── nxmc/                 # Management console
│   └── java/             # Java console code
├── nxalarm/              # Alarm notification tool
├── nxapisrv/             # API server
├── nxevent/              # Event sending tool
├── nxnotify/             # Notification tool
├── nxpush/               # Push data tool
├── nxshell/              # NXSL shell
└── nxtcpproxy/           # TCP proxy
```

## Management Console (nxmc)

### Build Commands

```bash
# Prerequisites: Build Java libraries first
mvn -f src/java-common/netxms-base/pom.xml install
mvn -f src/client/java/netxms-client/pom.xml install

# Build desktop client (standalone)
mvn -f src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone

# Build web client
mvn -f src/client/nxmc/java/pom.xml clean package -Pweb

# Run standalone client
java -jar src/client/nxmc/java/target/netxms-nxmc-standalone-*.jar

# Run web client in dev mode
mvn -f src/client/nxmc/java/pom.xml jetty:run -Pweb -Dnetxms.build.disablePlatformProfile=true
```

### Eclipse Setup

```bash
cd src/java-common/netxms-base && mvn install eclipse:eclipse && cd -
cd src/client/java/netxms-client && mvn install eclipse:eclipse && cd -
cd src/client/nxmc/java && mvn eclipse:eclipse -Pdesktop && cd -
# Import all three projects into Eclipse, run org.netxms.nxmc.Startup
```

### Key Packages

| Package | Description |
|---------|-------------|
| `org.netxms.nxmc` | Main application classes |
| `org.netxms.nxmc.base` | Base UI components |
| `org.netxms.nxmc.modules` | Feature modules |
| `org.netxms.nxmc.tools` | Utility classes |

### Module Structure

Each feature is a module in `org.netxms.nxmc.modules`:

```
modules/
├── alarms/           # Alarm management
├── assetmanagement/  # Asset tracking
├── businessservice/  # Business services
├── charts/           # Chart visualization
├── dashboards/       # Dashboard builder
├── datacollection/   # DCI configuration
├── events/           # Event configuration
├── logs/             # Log viewer
├── maps/             # Network maps
├── nxsl/             # Script editor
├── objects/          # Object browser
├── objecttools/      # Object tools
├── reporting/        # Reports
├── serverconfig/     # Server configuration
├── snmp/             # SNMP tools
├── users/            # User management
└── worldmap/         # Geographic maps
```

### SWT vs RWT

- **Desktop**: Uses SWT (Standard Widget Toolkit)
- **Web**: Uses RWT (RAP Widget Toolkit) - browser-based
- Code is mostly shared, with platform-specific handling where needed

## Java Client Library (netxms-client)

Location: `java/netxms-client/`

### Key Classes

| Class | Description |
|-------|-------------|
| `NXCSession` | Main session class for server communication |
| `NXCObjectModificationData` | Object modification container |
| `AbstractObject` | Base class for all objects |
| `Node` | Node object representation |
| `Interface` | Interface object representation |
| `DataCollectionItem` | DCI representation |
| `Alarm` | Alarm representation |
| `Event` | Event representation |

### Basic Usage

```java
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Node;

// Connect to server
NXCSession session = new NXCSession("server.example.com");
session.connect();
session.login("admin", "password");

// Get node by ID
Node node = (Node)session.findObjectById(nodeId);

// Get collected data
DciData data = session.getCollectedData(nodeId, dciId, from, to, 0, HistoricalDataType.PROCESSED);

// Disconnect
session.disconnect();
```

## C++ Client Library (libnxclient)

Location: `libnxclient/`

Used for C++ command-line tools and integrations.

### Key Classes

| Class | Description |
|-------|-------------|
| `NXCSession` | Server connection session |
| `AbstractObject` | Base object class |
| `Node` | Node representation |

## Command-Line Tools

### nxalarm
Sends alarm notifications.

### nxevent
Sends events to the server.

```bash
nxevent -s server -e EVENT_NAME -n node_id -p "param1" -p "param2"
```

### nxpush
Pushes metric values to the server.

```bash
nxpush -s server -n node_id "Metric.Name" value
```

### nxshell
Interactive NXSL scripting shell.

```bash
nxshell -s server
```

## Debugging

### Console Debug Output

Enable debug logging in the console:
1. Edit `~/.nxmc/nxmc.conf`
2. Set appropriate log levels

### Remote Debugging

For Eclipse:
1. Run with `-vmargs -agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=5005`
2. Connect Eclipse debugger to port 5005

## Related Components

- [netxms-base](../../java-common/netxms-base/) - Java common library
- [Server WebAPI](../server/webapi/) - REST API
- [libnetxms](../libnetxms/CLAUDE.md) - Core C++ library (for C++ tools)
