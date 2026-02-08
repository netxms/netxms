# Core Library (libnetxms)

> Shared guidelines: See [root CLAUDE.md](../../CLAUDE.md) for C++ development guidelines.

## Overview

`libnetxms` is the core C++ library used by all NetXMS components (server, agent, tools). It provides:
- String handling and containers
- Threading and synchronization
- Cryptography
- Network utilities
- Configuration parsing
- Logging infrastructure

## Directory Structure

This is a flat library with all source files in the root directory.

## Key Components

### String Handling

| Class/File | Description |
|------------|-------------|
| `string.cpp` | `String` class - dynamic string |
| `stringlist.cpp` | `StringList` - list of strings |
| `strmap.cpp` | `StringMap` - string-to-string map |

**Usage:**
```cpp
#include <nxcpapi.h>

String s = _T("Hello");
s.append(_T(" World"));

StringList list;
list.add(_T("item1"));
list.add(_T("item2"));

StringMap map;
map.set(_T("key"), _T("value"));
```

### Container Classes

| File | Class | Description |
|------|-------|-------------|
| `array.cpp` | `ObjectArray<T>` | Dynamic array of object pointers |
| `array.cpp` | `IntegerArray<T>` | Array of integers |
| `hashmapbase.cpp` | `HashMap<K,V>` | Hash map with string keys |
| `hashsetbase.cpp` | `HashSet<T>` | Hash set |
| `queue.cpp` | `ObjectQueue<T>` | Thread-safe queue |
| `squeue.cpp` | `SharedObjectQueue<T>` | Shared object queue |

**Ownership:**
```cpp
// Container owns objects (will delete them)
ObjectArray<MyClass> owned(16, 16, Ownership::True);
owned.add(new MyClass());  // Array will delete

// Container doesn't own objects
ObjectArray<MyClass> borrowed(16, 16, Ownership::False);
borrowed.add(existingPtr);  // Caller responsible for deletion
```

### Threading

| File | Class/Function | Description |
|------|----------------|-------------|
| `threads.cpp` | `ThreadCreateEx()` | Create a thread |
| `threads.cpp` | `Mutex` | Mutex class |
| `threads.cpp` | `Condition` | Condition variable |
| `threads.cpp` | `RWLock` | Read-write lock |
| `threads.cpp` | `LockGuard` | RAII mutex guard |
| `tp.cpp` | `ThreadPool*` | Thread pool functions |

**Usage:**
```cpp
// Mutex with RAII
Mutex m_lock;
{
   LockGuard lock(m_lock);
   // Protected code
}

// Thread pool
ThreadPoolExecute(g_threadPool, MyFunction, arg);
ThreadPoolScheduleRelative(g_threadPool, 5000, MyFunction, arg);  // 5 sec delay
```

### Memory Management

| File | Function | Description |
|------|----------|-------------|
| (in nms_util.h) | `MemAlloc(size)` | Allocate memory |
| (in nms_util.h) | `MemFree(ptr)` | Free memory |
| (in nms_util.h) | `MemAllocStruct<T>()` | Allocate struct |
| (in nms_util.h) | `MemAllocArray<T>(n)` | Allocate array |
| `mempool.cpp` | `MemoryPool` | Memory pool class |

**Usage:**
```cpp
// Allocate struct
MyStruct *s = MemAllocStruct<MyStruct>();

// Allocate array
int *arr = MemAllocArray<int>(100);

// Free
MemFree(s);
MemFree(arr);
```

### Cryptography

| File | Description |
|------|-------------|
| `crypto.cpp` | Encryption/decryption utilities |
| `hash.cpp` | Hash functions |
| `md5.cpp` | MD5 implementation |
| `sha1.cpp` | SHA-1 implementation |
| `sha2.cpp` | SHA-256/512 implementation |
| `cert.cpp` | Certificate handling |

### Network

| File | Description |
|------|-------------|
| `net.cpp` | Network utilities |
| `inetaddr.cpp` | `InetAddress` class |
| `macaddr.cpp` | `MacAddress` class |
| `icmp.cpp` | ICMP (ping) functions |
| `socket_listener.cpp` | Socket listener |
| `tls_conn.cpp` | TLS connections |

**Usage:**
```cpp
InetAddress addr = InetAddress::resolveHostName(_T("example.com"));
if (addr.isValid())
{
   // Use address
}
```

### NXCP Protocol

| File | Description |
|------|-------------|
| `nxcp.cpp` | NXCP message handling |
| `message.cpp` | `NXCPMessage` class |
| `msgrecv.cpp` | Message receiver |
| `msgwq.cpp` | Message wait queue |

### Configuration

| File | Description |
|------|-------------|
| `config.cpp` | Configuration file parsing |

### Logging

| File | Description |
|------|-------------|
| `log.cpp` | Logging infrastructure |
| `debug_tag_tree.cpp` | Debug tag management |

**Usage:**
```cpp
#define DEBUG_TAG _T("mymodule")

nxlog_debug_tag(DEBUG_TAG, 3, _T("Processing %s"), name);
nxlog_write(NXLOG_WARNING, _T("Warning message"));
```

### Utilities

| File | Description |
|------|-------------|
| `tools.cpp` | Various utility functions |
| `uuid.cpp` | UUID handling |
| `geolocation.cpp` | Geolocation class |
| `base64.cpp` | Base64 encoding/decoding |
| `base32.cpp` | Base32 encoding/decoding |
| `diff.cpp` | Text diff algorithm |
| `table.cpp` | `Table` class for tabular data |
| `bytestream.cpp` | `ByteStream` class |
| `procexec.cpp` | Process execution |
| `subproc.cpp` | Subprocess management |
| `vault.cpp` | Secret vault integration |

### Compression

| File | Description |
|------|-------------|
| `lz4.c` | LZ4 compression |
| `streamcomp.cpp` | Stream compression |
| `ztools.cpp` | Zlib utilities |

### Character Encoding

| File | Description |
|------|-------------|
| `unicode.cpp` | Unicode utilities |
| `cc_utf8.cpp` | UTF-8 conversion |
| `cc_ucs2.cpp` | UCS-2 conversion |
| `cc_ucs4.cpp` | UCS-4 conversion |
| `cc_mb.cpp` | Multibyte conversion |
| `iconv.cpp` | iconv wrapper |

### XML

| File | Description |
|------|-------------|
| `pugixml.cpp` | PugiXML library (embedded) |
| `xml.cpp` | XML utilities |

## TCHAR and Unicode

The library uses `TCHAR` for platform-independent character handling:
- On server: Always Unicode (WCHAR)
- On agent: Platform-dependent (WCHAR on Windows, char on Unix)

```cpp
// String literals
const TCHAR *str = _T("Hello World");

// Format strings
_sntprintf(buffer, size, _T("Value: %d"), value);

// String comparison
if (!_tcscmp(str1, str2))
   // Strings are equal
```

## Header Files

Main headers to include:

| Header | Provides |
|--------|----------|
| `nms_common.h` | Common definitions |
| `nms_util.h` | Utility classes and functions |
| `nxcpapi.h` | NXCP protocol and String class |

## Thread Safety

Most classes are NOT thread-safe by default. Use explicit synchronization:

```cpp
class MyClass
{
private:
   Mutex m_mutex;
   ObjectArray<Item> m_items;

public:
   void addItem(Item *item)
   {
      LockGuard lock(m_mutex);
      m_items.add(item);
   }
};
```

Thread-safe classes:
- `ObjectQueue<T>` - thread-safe by design
- `ConditionQueue<T>` - thread-safe by design

## Related Components

This library is used by:
- [Server](../server/CLAUDE.md)
- [Agent](../agent/CLAUDE.md)
- [Client](../client/CLAUDE.md)
- [NXSL](../libnxsl/CLAUDE.md)
- [Database](../db/CLAUDE.md)
- [SNMP](../snmp/CLAUDE.md)
