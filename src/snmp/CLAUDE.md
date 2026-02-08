# SNMP Library and Tools (libnxsnmp)

> Shared guidelines: See [root CLAUDE.md](../../CLAUDE.md) for C++ development guidelines.

## Overview

`libnxsnmp` provides SNMP protocol support for NetXMS:
- SNMP v1, v2c, and v3
- GET, GETNEXT, GETBULK, SET operations
- SNMP trap handling
- MIB parsing and OID manipulation

## Directory Structure

```
src/snmp/
├── libnxsnmp/        # SNMP protocol library
│   ├── main.cpp      # Library initialization
│   ├── pdu.cpp       # PDU encoding/decoding
│   ├── transport.cpp # SNMP transport
│   ├── variable.cpp  # Variable bindings
│   ├── ber.cpp       # BER encoding/decoding
│   ├── mib.cpp       # MIB handling
│   ├── security.cpp  # SNMPv3 security
│   └── ...
├── nxmibc/           # MIB compiler
├── nxsnmpget/        # SNMP GET tool
├── nxsnmpset/        # SNMP SET tool
└── nxsnmpwalk/       # SNMP WALK tool
```

## Key Classes

| Class | Description |
|-------|-------------|
| `SNMP_Transport` | SNMP transport (UDP/TCP) |
| `SNMP_UDPTransport` | UDP transport implementation |
| `SNMP_PDU` | SNMP Protocol Data Unit |
| `SNMP_Variable` | Variable binding |
| `SNMP_ObjectId` | OID representation |
| `SNMP_SecurityContext` | SNMPv3 security context |

## Basic SNMP Operations

### SNMP GET

```cpp
#include <nxsnmp.h>

// Create transport
SNMP_UDPTransport transport;
if (transport.createUDPTransport(_T("192.168.1.1"), 161) != SNMP_ERR_SUCCESS)
   return;

// Set community (v2c)
transport.setSecurityContext(new SNMP_SecurityContext(_T("public")));

// Create GET request
SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), SNMP_VERSION_2C);
request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.2.1.1.1.0")));  // sysDescr

// Send and receive
SNMP_PDU *response = nullptr;
uint32_t rc = transport.doRequest(&request, &response, 5000, 3);  // timeout, retries

if (rc == SNMP_ERR_SUCCESS && response != nullptr)
{
   SNMP_Variable *var = response->getVariable(0);
   if (var != nullptr)
   {
      TCHAR value[256];
      var->getValueAsString(value, 256);
      _tprintf(_T("sysDescr: %s\n"), value);
   }
   delete response;
}
```

### SNMP WALK

```cpp
// Walk callback
static uint32_t WalkCallback(SNMP_Variable *var, SNMP_Transport *transport, void *context)
{
   TCHAR oid[256], value[256];
   var->getName().toString(oid, 256);
   var->getValueAsString(value, 256);
   _tprintf(_T("%s = %s\n"), oid, value);
   return SNMP_ERR_SUCCESS;
}

// Perform walk
SNMP_ObjectId rootOid = SNMP_ObjectId::parse(_T(".1.3.6.1.2.1.2.2"));  // ifTable
transport.walk(&rootOid, WalkCallback, nullptr);
```

### SNMP SET

```cpp
SNMP_PDU request(SNMP_SET_REQUEST, SnmpNewRequestId(), SNMP_VERSION_2C);

// Add variable to set
SNMP_Variable *var = new SNMP_Variable(_T(".1.3.6.1.2.1.1.5.0"));  // sysName
var->setValueFromString(ASN_OCTET_STRING, _T("NewHostname"));
request.bindVariable(var);

SNMP_PDU *response = nullptr;
uint32_t rc = transport.doRequest(&request, &response, 5000, 3);
```

### SNMPv3

```cpp
// Create v3 security context
SNMP_SecurityContext *ctx = new SNMP_SecurityContext(
   _T("username"),           // Security name
   _T("authpassword"),       // Auth password
   _T("privpassword"),       // Privacy password
   SNMP_AUTH_SHA1,           // Auth protocol
   SNMP_ENCRYPT_AES_256);    // Encryption protocol

// Set security context level
ctx->setSecurityModel(SNMP_SECURITY_MODEL_USM);
ctx->setAuthoritativeEngine(SNMP_Engine(...));

transport.setSecurityContext(ctx);
```

## OID Handling

```cpp
// Parse OID from string
SNMP_ObjectId oid = SNMP_ObjectId::parse(_T(".1.3.6.1.2.1.1.1.0"));

// Build OID programmatically
SNMP_ObjectId oid;
oid.setValue(new uint32_t[8] {1, 3, 6, 1, 2, 1, 1, 1}, 8);

// OID comparison
if (oid1.compare(oid2) == 0)
   // OIDs are equal

// Check if one OID is prefix of another
if (oid.startsWith(prefix))
   // oid starts with prefix

// Get OID as string
TCHAR buffer[256];
oid.toString(buffer, 256);
```

## Variable Binding Types

| ASN Type | Constant | Description |
|----------|----------|-------------|
| Integer | `ASN_INTEGER` | Signed 32-bit integer |
| Counter32 | `ASN_COUNTER32` | 32-bit counter |
| Counter64 | `ASN_COUNTER64` | 64-bit counter |
| Gauge32 | `ASN_GAUGE32` | 32-bit gauge |
| TimeTicks | `ASN_TIMETICKS` | Time in centiseconds |
| OID | `ASN_OBJECT_ID` | Object identifier |
| Octet String | `ASN_OCTET_STRING` | Byte sequence |
| IP Address | `ASN_IP_ADDR` | IPv4 address |
| Null | `ASN_NULL` | Null value |

## Getting Values

```cpp
SNMP_Variable *var = response->getVariable(0);

// Get as specific type
int32_t intVal = var->getValueAsInt();
uint32_t uintVal = var->getValueAsUInt();
uint64_t uint64Val = var->getValueAsUInt64();
TCHAR strVal[256];
var->getValueAsString(strVal, 256);
InetAddress ipVal = var->getValueAsInetAddress();
MacAddress macVal = var->getValueAsMACAddr();

// Get raw value
size_t len;
const BYTE *raw = var->getValue(&len);
```

## Error Codes

| Code | Constant | Description |
|------|----------|-------------|
| 0 | `SNMP_ERR_SUCCESS` | Success |
| 1 | `SNMP_ERR_TIMEOUT` | Request timeout |
| 2 | `SNMP_ERR_PARAM` | Invalid parameter |
| 3 | `SNMP_ERR_SOCKET` | Socket error |
| 4 | `SNMP_ERR_COMM` | Communication error |
| 5 | `SNMP_ERR_PARSE` | Parse error |
| 6 | `SNMP_ERR_NO_OBJECT` | No such object |
| 7 | `SNMP_ERR_HOSTNAME` | Hostname resolution failed |
| 8 | `SNMP_ERR_BAD_OID` | Invalid OID |
| 9 | `SNMP_ERR_AGENT` | Agent error |
| 10 | `SNMP_ERR_BAD_TYPE` | Bad variable type |
| 11 | `SNMP_ERR_AUTH_FAILURE` | Authentication failure |
| 12 | `SNMP_ERR_ENGINE_ID` | Engine ID error |

## Command-Line Tools

### nxsnmpget

```bash
# SNMPv2c
nxsnmpget -c public 192.168.1.1 .1.3.6.1.2.1.1.1.0

# SNMPv3
nxsnmpget -v 3 -u user -a SHA -A authpass -x AES -X privpass 192.168.1.1 .1.3.6.1.2.1.1.1.0
```

### nxsnmpwalk

```bash
# Walk interface table
nxsnmpwalk -c public 192.168.1.1 .1.3.6.1.2.1.2.2
```

### nxsnmpset

```bash
# Set sysName
nxsnmpset -c private 192.168.1.1 .1.3.6.1.2.1.1.5.0 s "NewName"
```

### nxmibc

```bash
# Compile MIB file
nxmibc -o output.nxmib input.mib
```

## MIB Handling

```cpp
// Load MIB
SNMP_MIBObject *root = SNMPLoadMIBTree(_T("mibs/"));

// Find object by OID
SNMP_MIBObject *obj = SNMPFindMIBObject(root, oid);
if (obj != nullptr)
{
   _tprintf(_T("Name: %s\n"), obj->getName());
   _tprintf(_T("Description: %s\n"), obj->getDescription());
}
```

## Debugging

```bash
# Debug tags
nxlog_debug_tag(_T("snmp"), level, ...)       # General SNMP
nxlog_debug_tag(_T("snmp.trap"), level, ...)  # Trap handling
nxlog_debug_tag(_T("snmp.pdu"), level, ...)   # PDU encoding
```

## Related Components

- [libnetxms](../libnetxms/CLAUDE.md) - Core library
- [Server](../server/CLAUDE.md) - Uses SNMP for device monitoring
- [Server Drivers](../server/drivers/) - Use SNMP for device-specific data
- [Agent](../agent/CLAUDE.md) - Agent can proxy SNMP requests
