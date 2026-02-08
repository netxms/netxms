# Port Stop List Feature Design (NX-2863)

## Overview

The Port Stop List feature allows administrators to define lists of TCP and UDP ports that the NetXMS server should never access when communicating with devices. This is primarily intended to protect industrial controllers and other sensitive equipment that may crash when receiving unexpected network packets.

## Requirements

1. **Block all protocols** - Any TCP or UDP connection to specified ports is blocked
2. **Hierarchical configuration** - Stop lists can be configured at Zone, Container, Subnet, or Node level
3. **Node overrides** - If a node has its own stop list, it completely overrides any inherited lists
4. **Discovery integration** - Active network discovery respects stop lists

## Database Schema

**Table: `port_stop_list`**

| Column | Type | Description |
|--------|------|-------------|
| object_id | integer | NetObj ID (Zone, Container, Subnet, or Node) |
| id | integer | Entry order within object |
| port | integer | Port number (1-65535) |
| protocol | char(1) | 'T' = TCP, 'U' = UDP, 'B' = Both |

**Primary Key:** (object_id, id)

**Index:** idx_psl_object ON port_stop_list(object_id)

## Inheritance Model

```
Zone Level Stop List
        │
        ├──► Container/Collector Stop Lists
        │           │
        ├──► Subnet Stop Lists
        │           │
        └───────────┴──► Node
                         │
                         └──► If node has OWN list → USE IT (stops inheritance)
                              Otherwise → MERGE from all parents + Zone
```

**Algorithm:**
1. If the object has its own stop list configured → use only that list
2. Otherwise, collect ports from all parent containers, collectors, clusters, and subnets
3. Finally, add Zone-level stop list entries

## Core Implementation

**File:** `src/server/core/port_stop_list.cpp`

### Data Structures

```cpp
struct PortStopEntry
{
   uint32_t objectId;
   int32_t id;
   uint16_t port;
   char protocol;   // 'T' = TCP, 'U' = UDP, 'B' = Both
};

static StructArray<PortStopEntry> s_portStopList;
static Mutex s_portStopListLock(MutexType::FAST);
```

### Key Functions

| Function | Description |
|----------|-------------|
| `LoadPortStopList()` | Load entries from database at startup |
| `IsPortBlocked(objectId, port, tcp)` | Check if port is blocked for object |
| `GetEffectivePortStopList(objectId, tcpPorts, udpPorts)` | Get full effective list with inheritance |
| `GetEffectivePortStopListForZone(zoneUIN, tcpPorts, udpPorts)` | Get zone-level list for discovery |
| `PortStopListToMessage(objectId, msg)` | Serialize to NXCP message |
| `UpdatePortStopList(request, objectId)` | Update from NXCP message |
| `DeletePortStopList(objectId)` | Remove entries when object is deleted |

## Integration Points

The port stop list is checked before any network communication:

| Protocol | File | Function |
|----------|------|----------|
| SNMP (UDP 161) | node.cpp | `Node::createSnmpTransport()` |
| Agent (TCP 4700) | node.cpp | `Node::createAgentConnection()` |
| Modbus TCP (TCP 502) | node.cpp | `Node::createModbusTransport()` |
| EtherNet/IP (TCP 44818) | node.cpp | `Node::getMetricFromEtherNetIP()` |
| SSH (TCP 22) | datacoll.cpp | Data collection switch case |
| Discovery | discovery.cpp | Port filtering before scans |

## NXCP Protocol

### Command Codes

| Code | Value | Description |
|------|-------|-------------|
| CMD_GET_PORT_STOP_LIST | 0x01FA | Get stop list for object |
| CMD_UPDATE_PORT_STOP_LIST | 0x01FB | Update stop list for object |

### Notification

| Code | Value | Description |
|------|-------|-------------|
| NX_NOTIFY_PORT_STOP_LIST_CHANGED | 66 | Stop list was modified |

### Variable IDs

| Variable | Value | Description |
|----------|-------|-------------|
| VID_PORT_STOP_COUNT | 913 | Number of entries in list |
| VID_PORT_STOP_LIST_BASE | 0x77000000 | Base for entry fields |

### Message Format

For each entry (10 fields reserved per entry):
- VID_PORT_STOP_LIST_BASE + (i * 10) + 0: port (uint16)
- VID_PORT_STOP_LIST_BASE + (i * 10) + 1: protocol (int16, char value)
- Fields 2-9: Reserved for future use

## Files Modified/Created

| File | Change |
|------|--------|
| sql/schema.in | Added table definition |
| src/server/core/port_stop_list.cpp | **NEW** - Core implementation |
| src/server/include/nms_core.h | Function declarations |
| src/server/core/Makefile.am | Added source file |
| src/server/core/nxcore.vcxproj | Added source file |
| src/server/core/nxcore.vcxproj.filters | Added source file |
| src/server/core/node.cpp | Added blocking checks |
| src/server/core/datacoll.cpp | Added SSH blocking check |
| src/server/core/discovery.cpp | Added port filtering |
| src/server/core/session.cpp | Added NXCP handlers |
| src/server/core/main.cpp | Added LoadPortStopList() call |
| include/nms_cscp.h | Added command and VID codes |
| include/nxcldefs.h | Added notification code |

## Remaining Work

### Database Upgrade Script
- File: `src/server/tools/nxdbmgr/upgrade_vXX.cpp`
- Add table creation for existing installations

### NXSL Methods
- File: `src/server/core/nxsl_classes.cpp`
- `$node->isPortBlocked(port, "tcp"|"udp")` - Check if port is blocked
- `$node->getBlockedPorts("tcp"|"udp")` - Get array of blocked ports

### Java Client API
- `NXCSession.getPortStopList(objectId)` - Retrieve stop list
- `NXCSession.updatePortStopList(objectId, entries)` - Update stop list
- New class: `PortStopEntry`

### Management Console UI
- Add "Port Stop List" tab to object properties for Zone, Container, Subnet, Node
- UI: Table with Port, Protocol (TCP/UDP/Both), Add/Remove buttons

## Usage Examples

### Protecting an Industrial Controller

1. Navigate to the Node object for the controller
2. Open Properties → Port Stop List tab
3. Add entries:
   - Port 22, Protocol: TCP (block SSH)
   - Port 44818, Protocol: TCP (block EtherNet/IP if not used)
4. Save changes

### Zone-wide Protection

1. Navigate to the Zone object
2. Open Properties → Port Stop List tab
3. Add default blocked ports for all nodes in the zone
4. Individual nodes can override by defining their own list

## Debug Tags

- `port.stoplist` - Port stop list operations and lookups
