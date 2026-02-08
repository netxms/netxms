# NetXMS C++ Developer Guide

## Table of Contents
1. [Introduction](#introduction)
2. [Architecture Overview](#architecture-overview)
3. [Core C++ Libraries](#core-c-libraries)
4. [String Handling](#string-handling)
5. [Container Classes](#container-classes)
6. [Specialized Data Types](#specialized-data-types)
7. [Threading and Concurrency](#threading-and-concurrency)
8. [Memory Management](#memory-management)
9. [Common Utility Functions](#common-utility-functions)
10. [Coding Style and Conventions](#coding-style-and-conventions)
11. [Platform Abstractions](#platform-abstractions)
12. [Server-Side Data Structures](#server-side-data-structures)
13. [Best Practices](#best-practices)

## Introduction

NetXMS is an enterprise-grade, open-source network and infrastructure monitoring and management solution written primarily in C++. This guide provides new C++ developers with essential information about the codebase structure, core libraries, patterns, and conventions used throughout the NetXMS project.

**Key Facts:**
- **Current Version:** 5.3.0
- **License:** GNU General Public License for most of the code (see mode in LICENSE.md)
- **Primary Language:** C++ with Java components for UI
- **Maximum C++ Version:** C++11
- **Architecture:** Distributed system with server, agent, and client components

### C++ Version Requirements

NetXMS requires **C++11 as the maximum standard** for development:

- **Maximum standard:** C++11 
- **Compiler Requirements:**
  - GCC 4.8+ (full C++11 support)
  - Clang 3.3+ (full C++11 support)  
  - Visual Studio 2015+ (full C++11 support)

### Key C++11 Features Used
- Smart pointers (`std::shared_ptr`, `std::unique_ptr`)
- `nullptr` keyword (`NULL` is deprecated)
- `auto` type deduction (do not overuse)
- Variadic templates
- Move semantics
- Lambda expressions (in newer code)

## Architecture Overview

### Core Components

```
NetXMS System Architecture
├── Server (netxmsd) - Central management and monitoring
├── Agent (nxagentd) - Data collection from monitored systems  
├── Database - Configuration and data storage
└── Client Applications - Management interfaces
```

### Key Directories

```
/src/
├── libnetxms/           # Core utility library (THIS IS CRUCIAL)
├── libnxsl/             # NetXMS Scripting Language
├── libnxdb/             # Database abstraction layer
├── libnxcc/             # Client-server communication
├── server/core/         # Core server functionality
├── agent/core/          # Agent core functionality
└── client/              # Client libraries and applications
```

## Core C++ Libraries

### 1. libnetxms - Foundation Library

**Location:** `src/libnetxms/`  
**Header:** `include/nms_common.h`, `include/nms_util.h`, `include/nms_threads.h`

This is the **most important library** for NetXMS C++ development. It provides:

- Cross-platform abstractions
- Memory management utilities
- String handling classes
- Container classes
- Threading primitives
- Network utilities
- Cryptographic functions

**Key Files:**
- `nms_common.h` - Common definitions and macros (memory management, platform abstractions)
- `nms_util.h` - Container classes, string handling (String, StringList, Array, HashMap...). Memory optimised for small object allocation, and have convinient methods (comparing to std::)
- `nms_threads.h` - Threading abstractions

### 2. libnxsl - Scripting Language

**Location:** `src/libnxsl/`  
**Purpose:** NetXMS Scripting Language interpreter and virtual machine

### 3. libnxdb - Database Layer

**Location:** `src/libnxdb/`  
**Purpose:** Database abstraction supporting PostgreSQL, MySQL, Oracle, MSSQL, SQLite

### 4. libnxcc - Communication

**Location:** `src/libnxcc/`  
**Purpose:** Client-server communication using NXCP protocol

## String Handling

NetXMS provides a comprehensive string handling system with Unicode support.

### String Class

```cpp
class LIBNETXMS_EXPORTABLE String
{
private:
   TCHAR *m_buffer;
   size_t m_length;
   TCHAR m_internalBuffer[STRING_INTERNAL_BUFFER_SIZE];  // Small string optimization

public:
   String();                                    // Empty string
   String(const TCHAR *init);                   // From C string
   String(const String& src);                   // Copy constructor
   String(String&& src);                        // Move constructor
   String(const TCHAR *init, ssize_t len, Ownership takeOwnership);
   
   // Operators
   String operator +(const String &right) const;
   String operator +(const TCHAR *right) const;
   String& operator =(const String &right);
   String& operator +=(const String &right);
   
   // Methods
   String substring(size_t start, ssize_t len = -1) const;
   void trim();
   void replace(const TCHAR *substr, const TCHAR *newSubstr);
   bool equals(const String &str) const;
   bool startsWith(const TCHAR *str) const;
   
   // Accessors
   const TCHAR *cstr() const { return m_buffer; }
   size_t length() const { return m_length; }
   bool isEmpty() const { return m_length == 0; }
};
```

**Key Features:**
- **Small String Optimization:** Strings shorter than internal buffer size avoid heap allocation
- **Move Semantics:** Efficient string transfers
- **Unicode Support:** Full TCHAR/Unicode abstraction
- **Memory Efficient:** Optimized memory usage patterns

### String Utilities

```cpp
// Null-safe string checking
const TCHAR *CHECK_NULL(const TCHAR *x);      // Returns "(null)" for NULL
const TCHAR *CHECK_NULL_EX(const TCHAR *x);   // Returns "" for NULL

// Memory-based string functions
char *MemCopyStringA(const char *src);         // Allocate and copy ASCII string
WCHAR *MemCopyStringW(const WCHAR *src);       // Allocate and copy wide string
TCHAR *MemCopyString(const TCHAR *src);        // Allocate and copy TCHAR string
```

### StringList Class

```cpp
class LIBNETXMS_EXPORTABLE StringList
{
public:
   void add(const TCHAR *str);
   void addPreallocated(TCHAR *str);           // Takes ownership
   const TCHAR *get(int index) const;
   int size() const;
   void sort(bool ascending = true);
   String join(const TCHAR *separator) const;
   void splitAndAdd(const TCHAR *str, const TCHAR *separator);
   
   // From various sources
   void addFromMessage(const NXCPMessage& msg, uint32_t baseId);
   void addFromJson(json_t *root);
};
```

## Container Classes

NetXMS uses custom container classes optimized for network management tasks rather than relying heavily on STL.

### Array Classes

#### ObjectArray Template
```cpp
template<class T>
class ObjectArray : public Array
{
public:
   ObjectArray(int initialSize = 0, int growBy = 16, Ownership owner = Ownership::True);
   
   void add(T *object);
   void insert(int index, T *object);
   void remove(int index);
   void replace(int index, T *object);
   
   T *get(int index) const;
   T *operator[](int index) const { return get(index); }
   
   void sort(int (*compare)(const T *, const T *));
   
   // Iterators
   Iterator<T> begin() { return Iterator<T>(this); }
   Iterator<T> end() { return Iterator<T>(); }
};
```

#### IntegerArray Template
```cpp
template<typename T>
class IntegerArray : public Array
{
public:
   void add(T value);
   T get(int index) const;
   void sortAscending();
   void sortDescending();
};
```

**Usage Example:**
```cpp
ObjectArray<Node> nodes(64);         // Array of Node pointers
nodes.add(new Node());
nodes.sort(NodeComparator);

IntegerArray<uint32_t> ids;          // Array of integers
ids.add(1001);
ids.sortAscending();
```

### Hash-Based Containers

#### HashMap Template
```cpp
template<class T>
class HashMap : public HashMapBase
{
public:
   HashMap(Ownership owner = Ownership::True);
   
   void set(const TCHAR *key, T *object);
   T *get(const TCHAR *key) const;
   bool contains(const TCHAR *key) const;
   void remove(const TCHAR *key);
   
   void forEach(std::function<EnumerationCallbackResult (const TCHAR *, const T *)> callback) const;
   
   // Iterator support
   Iterator<T> begin();
   Iterator<T> end();
};
```

#### StringMap Class
```cpp
class LIBNETXMS_EXPORTABLE StringMap : public StringMapBase
{
public:
   void set(const TCHAR *key, const TCHAR *value);
   const TCHAR *get(const TCHAR *key) const;
   const TCHAR *get(const TCHAR *key, const TCHAR *defaultValue) const;
   
   bool contains(const TCHAR *key) const;
   void remove(const TCHAR *key);
   
   StringList *keys() const;
   StringList *values() const;
};
```

### Queue Classes

#### Thread-Safe Queue
```cpp
template<class T>
class ObjectQueue : public Queue
{
public:
   void put(T *object);
   T *get();                         // Blocking get
   T *getOrBlock(uint32_t timeout);  // Timeout-based get
   
   void setShutdownMode();           // Signal shutdown
   void clear();
};
```

### Memory Pool Classes

#### ObjectMemoryPool Template
```cpp
template<class T>
class ObjectMemoryPool
{
public:
   ObjectMemoryPool(size_t blockSize = 64);
   
   T *allocate();
   void destroy(T *object);
   void reset();                     // Reset without destroying objects
   void clear();                     // Reset and destroy all objects
};
```

**Usage Example:**
```cpp
ObjectMemoryPool<DataPoint> dataPool(128);
DataPoint *dp = dataPool.allocate();
// Use dp...
dataPool.destroy(dp);
```

## Specialized Data Types

NetXMS provides specialized classes for common network monitoring data types that handle validation, conversion, and network-specific operations.

### UUID Class

The `uuid` class provides universally unique identifier handling with various constructor options and formatting capabilities. It's a wrapper around the internal `uuid_t` type.

```cpp
#include "uuid.h"

class uuid 
{
private:
   uuid_t m_value;                     // Internal UUID storage

public:
   // Constructors
   uuid();                             // Create null UUID (all zeros)
   uuid(const uuid_t v);               // Create from uuid_t value
   
   // Comparison
   int compare(const uuid& u) const;   // Compare UUIDs (-1, 0, 1)
   bool equals(const uuid& u) const;   // Check equality
   
   // Access methods
   const uuid_t& getValue() const;     // Get raw uuid_t reference
   bool isNull() const;                // Check if UUID is null (all zeros)
   
   // String conversion
   TCHAR *toString(TCHAR *buffer) const;   // Convert to TCHAR string
   String toString() const;                // Return as String object
   char *toStringA(char *buffer) const;    // Convert to ASCII string
   json_t *toJson() const;                 // Convert to JSON string
   
   // Type conversion operator
   operator const uuid_t&() const;    // Implicit conversion to uuid_t
   
   // Static methods
   static uuid generate();             // Generate new random UUID
   static uuid parse(const TCHAR *s);  // Parse from TCHAR string (NULL_UUID on error)
   static uuid parseA(const char *s);  // Parse from ASCII string (NULL_UUID on error)
   
   // Static constants
   static const uuid NULL_UUID;       // Predefined null UUID (all zeros)
};
```

**Usage Examples:**
```cpp
// Generate new UUID
uuid id = uuid::generate();

// Parse from string
uuid parsed = uuid::parse(_T("550e8400-e29b-41d4-a716-446655440000"));
uuid parsedA = uuid::parseA("550e8400-e29b-41d4-a716-446655440000");

// Check for parsing errors (returns NULL_UUID on error)
if (parsed.isNull()) {
   nxlog_write(NXLOG_ERROR, _T("Failed to parse UUID string"));
} else {
   nxlog_write(NXLOG_INFO, _T("Successfully parsed UUID"));
}

// Convert to string
TCHAR buffer[64];
id.toString(buffer);

// String object conversion
String uuidStr = id.toString();

// ASCII string conversion
char asciiBuffer[64];
id.toStringA(asciiBuffer);

// Check for null UUID
if (id.isNull()) {
   nxlog_write(NXLOG_WARNING, _T("UUID is null"));
}

// Compare UUIDs
uuid other = uuid::generate();
if (id.equals(other)) {
   nxlog_write(NXLOG_INFO, _T("UUIDs are equal"));
}

// Three-way comparison
int result = id.compare(other);
if (result < 0) {
   nxlog_write(NXLOG_INFO, _T("First UUID is less than second"));
} else if (result > 0) {
   nxlog_write(NXLOG_INFO, _T("First UUID is greater than second"));
} else {
   nxlog_write(NXLOG_INFO, _T("UUIDs are equal"));
}

// Access raw uuid_t (for C library functions)
const uuid_t& rawUuid = id.getValue();
// or use implicit conversion
some_c_function(id);  // uuid converts to uuid_t automatically

// Create from existing uuid_t
uuid_t existingUuid;
_uuid_generate(existingUuid);
uuid fromExisting(existingUuid);

// JSON conversion
json_t *jsonUuid = id.toJson();

// Use null UUID constant
uuid nullUuid = uuid::NULL_UUID;
if (someUuid.equals(uuid::NULL_UUID)) {
   nxlog_write(NXLOG_INFO, _T("UUID is the null UUID"));
}
```

### NetAddr Class

The `InetAddress` class handles IP addresses with support for both IPv4 and IPv6, including subnet operations and various address manipulations.

```cpp
#include "inet_addr.h"

class InetAddress 
{
private:
   short m_maskBits;                 // Subnet mask bits
   short m_family;                   // Address family (AF_INET/AF_INET6)
   union {
      uint32_t v4;                   // IPv4 address
      BYTE v6[16];                   // IPv6 address
   } m_addr;

public:
   // Constructors
   InetAddress();                                    // Default (invalid address)
   InetAddress(uint32_t addr);                      // From IPv4 address (32-bit mask)
   InetAddress(uint32_t addr, uint32_t mask);       // From IPv4 with mask
   InetAddress(const BYTE *addr, int maskBits = 128); // From IPv6 bytes
   
   // Address information
   int getFamily() const;              // AF_INET or AF_INET6
   bool isValid() const;              // Check if address family is valid
   bool isValidUnicast() const;       // Check if valid unicast address
   bool isAnyLocal() const;           // Check if 0.0.0.0 or ::
   bool isLoopback() const;           // Check if 127.x.x.x or ::1
   bool isMulticast() const;          // Check if multicast address
   bool isBroadcast() const;          // Check if IPv4 broadcast (255.255.255.255)
   bool isLinkLocal() const;          // Check if link-local address
   
   // Address access
   uint32_t getAddressV4() const;     // Get IPv4 as 32-bit (0 if not IPv4)
   const BYTE *getAddressV6() const;  // Get IPv6 bytes (zeros if not IPv6)
   
   // Mask operations
   void setMaskBits(int m);           // Set subnet mask bits
   int getMaskBits() const;           // Get subnet mask bits
   int getHostBits() const;           // Get host portion bits
   
   // String conversion
   String toString() const;           // Convert to string
   TCHAR *toString(TCHAR *buffer) const;
   char *toStringA(char *buffer) const;  // ASCII version
   
   // Subnet operations
   bool contains(const InetAddress &a) const;        // Check if address is in subnet
   bool sameSubnet(const InetAddress &a) const;      // Check if in same subnet
   InetAddress getSubnetAddress() const;             // Get subnet address
   InetAddress getSubnetBroadcast() const;           // Get subnet broadcast
   bool isSubnetBroadcast() const;                   // Check if subnet broadcast
   bool isSubnetBroadcast(int maskBits) const;       // Check with specific mask
   
   // Comparison
   bool equals(const InetAddress &a) const;
   int compareTo(const InetAddress &a) const;
   bool inRange(const InetAddress& start, const InetAddress& end) const;
   
   // Utilities
   json_t *toJson() const;
   void toOID(uint32_t *oid) const;                  // Convert to SNMP OID
   TCHAR *getHostByAddr(TCHAR *buffer, size_t buflen) const; // Reverse DNS
   struct sockaddr *fillSockAddr(SockAddrBuffer *buffer, uint16_t port = 0) const;
   
   // Static utilities
   static InetAddress INVALID;       // Invalid address constant
   static InetAddress LOOPBACK;      // Loopback address
   static InetAddress NONE;          // Zero address
   
   static InetAddress resolveHostName(const TCHAR *hostname, int af = AF_UNSPEC);
   static InetAddress parse(const TCHAR *str);
   static InetAddress parse(const TCHAR *addrStr, const TCHAR *maskStr);
   static InetAddress createFromSockaddr(const struct sockaddr *s);
};
```

**Usage Examples:**
```cpp
// Create from numeric IPv4
InetAddress addr(0xC0A80164);  // 192.168.1.100

// Create IPv4 with mask
InetAddress subnet(0xC0A80100, 0xFFFFFF00);  // 192.168.1.0/24

// Create from string
InetAddress parsed = InetAddress::parse(_T("192.168.1.100/24"));
InetAddress ipv6 = InetAddress::parse(_T("2001:db8::1/64"));

// Validate and convert
if (addr.isValid()) {
   TCHAR buffer[64];
   addr.toString(buffer);
   nxlog_write(NXLOG_INFO, _T("Address: %s"), buffer);
}

// Address type checking
if (addr.isLoopback()) {
   nxlog_write(NXLOG_INFO, _T("Loopback address"));
}
if (addr.isMulticast()) {
   nxlog_write(NXLOG_INFO, _T("Multicast address"));
}
if (addr.isBroadcast()) {
   nxlog_write(NXLOG_INFO, _T("IPv4 broadcast (255.255.255.255)"));
}

// Subnet operations
InetAddress network = addr.getSubnetAddress();
InetAddress broadcast = addr.getSubnetBroadcast();

// Check subnet membership
InetAddress other = InetAddress::parse(_T("192.168.1.200"));
other.setMaskBits(24);  // Set same mask
if (addr.sameSubnet(other)) {
   nxlog_write(NXLOG_INFO, _T("Addresses are in same subnet"));
}

// Check if address is contained in subnet
if (subnet.contains(addr)) {
   nxlog_write(NXLOG_INFO, _T("Address is in subnet"));
}

// Hostname resolution
InetAddress resolved = InetAddress::resolveHostName(_T("www.example.com"));
if (resolved.isValid()) {
   nxlog_write(NXLOG_INFO, _T("Resolved to: %s"), resolved.toString().cstr());
}

// Get raw address data
uint32_t ipv4Raw = addr.getAddressV4();  // 0 if not IPv4
const BYTE *ipv6Raw = addr.getAddressV6(); // zeros if not IPv6

// Mask operations
addr.setMaskBits(24);
int hostBits = addr.getHostBits();  // 8 for /24 IPv4
```

### MacAddr Class

The `MacAddress` class handles MAC (Media Access Control) addresses with support for various formats and validation. It inherits from `GenericId<8>` providing storage for up to 8 bytes.

```cpp
#include "macaddr.h"

class MacAddress : public GenericId<8>
{
public:
   // Constructors
   MacAddress(size_t length = 0);                  // Default with specific length
   MacAddress(const BYTE *value, size_t length);   // From byte array
   MacAddress(const MacAddress& src);              // Copy constructor
   
   // Assignment operator
   MacAddress& operator=(const MacAddress& src);
   
   // Parsing from strings
   static MacAddress parse(const char *str, bool partialMac = false);
   static MacAddress parse(const WCHAR *str, bool partialMac = false);
   
   // Validation and type checking
   bool isValid() const;                   // Check if valid (not null)
   bool isBroadcast() const;              // Check if FF:FF:FF:FF:FF:FF (6-byte only)
   bool isMulticast() const;              // Check if multicast (first bit set, not broadcast)
   bool equals(const MacAddress &a) const;
   bool equals(const BYTE *value, size_t length = 6) const;
   
   // String conversion
   TCHAR *toString(TCHAR *buffer, MacAddressNotation notation = MacAddressNotation::COLON_SEPARATED) const;
   String toString(MacAddressNotation notation = MacAddressNotation::COLON_SEPARATED) const;
   
   // Static constants
   static const MacAddress NONE;          // All zeros MAC
   static const MacAddress ZERO;          // Alias for NONE
};

// MAC Address Notation Options
enum class MacAddressNotation
{
   FLAT_STRING,          // 001122334455
   COLON_SEPARATED,      // 00:11:22:33:44:55 (default)
   HYPHEN_SEPARATED,     // 00-11-22-33-44-55
   DOT_SEPARATED,        // 0011.2233.4455
   BYTE_PAIRS,           // Custom byte pair format
   DECIMAL_DOT           // Decimal format with dots
};
```

**Usage Examples:**
```cpp
// Parse from various string formats
MacAddress mac1 = MacAddress::parse("00:11:22:33:44:55");
MacAddress mac2 = MacAddress::parse("00-11-22-33-44-55");
MacAddress mac3 = MacAddress::parse("001122334455");
MacAddress mac4 = MacAddress::parse("0011.2233.4455");

// Parse partial MAC (for EUI-64, etc.)
MacAddress partialMac = MacAddress::parse("00:11:22", true);

// Create from byte array
BYTE macBytes[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
MacAddress macFromBytes(macBytes, 6);

// Validate MAC address
if (mac1.isValid()) {
   nxlog_write(NXLOG_INFO, _T("Valid MAC address"));
}

// Convert to different string formats
TCHAR buffer[32];
mac1.toString(buffer, MacAddressNotation::COLON_SEPARATED);      // "00:11:22:33:44:55"
mac1.toString(buffer, MacAddressNotation::HYPHEN_SEPARATED);     // "00-11-22-33-44-55"
mac1.toString(buffer, MacAddressNotation::FLAT_STRING);          // "001122334455"
mac1.toString(buffer, MacAddressNotation::DOT_SEPARATED);        // "0011.2233.4455"

// String object conversion (default is colon-separated)
String macStr = mac1.toString();  // "00:11:22:33:44:55"

// Check for special addresses
if (mac1.isBroadcast()) {
   nxlog_write(NXLOG_INFO, _T("Ethernet broadcast MAC (FF:FF:FF:FF:FF:FF)"));
}
if (mac1.isMulticast()) {
   nxlog_write(NXLOG_INFO, _T("Multicast MAC (first bit set, not broadcast)"));
}

// Comparison
if (mac1.equals(mac2)) {
   nxlog_write(NXLOG_INFO, _T("MAC addresses are equal"));
}

// Compare with raw bytes
BYTE testBytes[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
if (mac1.equals(testBytes, 6)) {
   nxlog_write(NXLOG_INFO, _T("MAC matches byte array"));
}

// Access raw data (inherited from GenericId<8>)
const BYTE *rawMac = mac1.getValue();
size_t length = mac1.length();

// Copy constructor and assignment
MacAddress macCopy = mac1;
MacAddress macAssigned;
macAssigned = mac1;
```

### Best Practices for Specialized Data Types

**UUID Usage:**
- Use `generate()` static method to create new UUIDs
- Use `parse()` or `parseA()` static methods for string conversion (returns NULL_UUID on error)
- Always check `isNull()` when parsing UUIDs from external sources
- Use `equals()` for equality comparison or `compare()` for ordering
- Use the String-returning `toString()` for temporary string usage
- Access raw `uuid_t` with `getValue()` or implicit conversion for C library functions

**InetAddress Usage:**
- Always validate addresses before use with `isValid()`
- Use `parse()` static method for string-to-address conversion
- Set appropriate mask bits with `setMaskBits()` for subnet operations
- Use `sameSubnet()` and `contains()` for network topology analysis
- Check address types with `isLoopback()`, `isMulticast()`, `isBroadcast()` etc.

**MacAddress Usage:**
- Use static `parse()` method for string conversion with automatic format detection
- Validate MAC addresses from external sources with `isValid()`
- Use `isBroadcast()` to check for Ethernet broadcast (FF:FF:FF:FF:FF:FF, 6-byte only)
- Use `isMulticast()` to identify multicast MACs (first bit set, excluding broadcast)
- Choose appropriate `MacAddressNotation` for string output format
- Support for partial MACs and various length addresses through `GenericId<8>` inheritance

## Threading and Concurrency

NetXMS provides cross-platform threading abstractions supporting Windows, pthreads, and GNU Pth.

### Thread Management

```cpp
/* Most actions in threads in server, agent should be done in thread pools */

/* Thread pool functions */
void LIBNETXMS_EXPORTABLE ThreadPoolExecute(ThreadPool *p, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolExecuteSerialized(ThreadPool *p, const TCHAR *key, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleAbsolute(ThreadPool *p, time_t runTime, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleAbsoluteMs(ThreadPool *p, int64_t runTime, ThreadPoolWorkerFunction f, void *arg);
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleRelative(ThreadPool *p, uint32_t delay, ThreadPoolWorkerFunction f, void *arg);
...

// Thread functions
THREAD ThreadCreateEx(ThreadFunction startAddress, int stackSize, void *args);
bool ThreadCreate(ThreadFunction startAddress, int stackSize, void *args);  // Fire-and-forget
static bool ThreadJoin(THREAD hThread)

// Sleep functions
void ThreadSleep(int seconds);
void ThreadSleepMs(uint32_t milliseconds);
```

### Synchronization Primitives

#### Mutex Class
```cpp
class LIBNETXMS_EXPORTABLE Mutex
{
public:
   Mutex(MutexType type = MutexType::NORMAL);
   ~Mutex();
   
   void lock();
   bool tryLock();
   void unlock();
};
```

#### LockGuard RAII Helper
```cpp
class LIBNETXMS_EXPORTABLE LockGuard
{
public:
   LockGuard(Mutex& mutex) : m_mutex(mutex) { m_mutex.lock(); }
   ~LockGuard() { m_mutex.unlock(); }
   
private:
   Mutex& m_mutex;
};
```

#### Condition Variables
```cpp
class LIBNETXMS_EXPORTABLE Condition
{
public:
   Condition(bool broadcast = false);
   ~Condition();
   
   void set();
   void pulse();
   void reset();
   bool wait(uint32_t timeout = INFINITE);
};
```

#### Reader-Writer Locks
```cpp
class LIBNETXMS_EXPORTABLE RWLock
{
public:
   RWLock();
   ~RWLock();
   
   void readLock();
   void writeLock();
   void unlock();
};
```

**Thread-Safe Pattern Example:**
```cpp
class ThreadSafeCounter
{
private:
   Mutex m_mutex;
   int m_value;
   
public:
   ThreadSafeCounter() : m_value(0) {}
   
   void increment()
   {
      LockGuard lock(m_mutex);  // RAII lock
      m_value++;
   }
   
   int getValue()
   {
      LockGuard lock(m_mutex);
      return m_value;
   }
};
```

## Memory Management

NetXMS provides a comprehensive memory management system with safety features and optimizations.

### Core Memory Functions

```cpp
// Basic allocation
void *MemAlloc(size_t size);
void *MemAllocZeroed(size_t size);

// Type-safe allocation
template<typename T> T *MemAllocStruct();                    // Single zero-initialized struct
template<typename T> T *MemAllocArray(size_t count);         // Zero-initialized array
template<typename T> T *MemAllocArrayNoInit(size_t count);   // Uninitialized array

// String allocation
char *MemAllocStringA(size_t size);
WCHAR *MemAllocStringW(size_t size);
TCHAR *MemAllocString(size_t size);

// Reallocation
template<typename T> T *MemRealloc(T *p, size_t size);        // Free on failure
template<typename T> T *MemReallocNoFree(T *p, size_t size);  // Don't free on failure

// Deallocation
void MemFree(void *p);                                       // Null-safe
template<typename T> void MemFreeAndNull(T* &p);            // Free and nullify
```

### Memory Copy Functions

```cpp
void *MemCopyBlock(const void *data, size_t size);
template<typename T> T *MemCopyArray(const T *data, size_t count);

char *MemCopyStringA(const char *src);
WCHAR *MemCopyStringW(const WCHAR *src);
TCHAR *MemCopyString(const TCHAR *src);
```

### Smart Pointers

NetXMS uses standard smart pointers with internal implementation on platforms that do not support C++11.

```cpp
// Standard smart pointers (when available)
#include <memory>
using std::shared_ptr;
using std::unique_ptr;
using std::weak_ptr;
using std::make_shared;
using std::make_unique;
```

### Memory Pool Pattern

```cpp
class LIBNETXMS_EXPORTABLE MemoryPool
{
public:
   MemoryPool(size_t initialSize = 8192);
   ~MemoryPool();
   
   void *allocate(size_t size);
   char *copyString(const char *str);
   WCHAR *copyStringW(const WCHAR *str);
   
   void reset();  // Reset pool, making all allocations invalid
};
```

**Usage Example:**
```cpp
MemoryPool pool;
char *str1 = pool.copyString("Hello");
char *str2 = pool.copyString("World");
// ... use strings
pool.reset();  // All strings become invalid
```

## Common Utility Functions

### Cryptography and Hashing

```cpp
// Hash functions
void CalculateMD5Hash(const void *data, size_t len, BYTE *hash);
void CalculateSHA1Hash(const void *data, size_t len, BYTE *hash);
void CalculateSHA256Hash(const void *data, size_t len, BYTE *hash);

// String hash helpers
String CalculateMD5HashStr(const void *data, size_t len);
String CalculateSHA1HashStr(const void *data, size_t len);

// Password hashing
String CreatePasswordHash(const TCHAR *password, PasswordHashType type);
bool ValidatePasswordHash(const TCHAR *password, const TCHAR *hash, PasswordHashType *type);
```

### Configuration Management

Common class to read configuration files in ini or XML format

```cpp
class LIBNETXMS_EXPORTABLE Config
{
public:
   bool loadConfigFile(const TCHAR *fileName, bool wideCharPath);
   
   ConfigEntry *getEntry(const TCHAR *name) const;
   const TCHAR *getValue(const TCHAR *name, const TCHAR *defaultValue = nullptr) const;
   int32_t getValueAsInt(const TCHAR *name, int32_t defaultValue = 0) const;
   bool getValueAsBoolean(const TCHAR *name, bool defaultValue = false) const;
   
   StringList *getKeys() const;
};

class LIBNETXMS_EXPORTABLE ConfigEntry
{
public:
   const TCHAR *getValue(int index = 0) const;
   int32_t getValueAsInt(int index = 0, int32_t defaultValue = 0) const;
   uint32_t getValueAsUInt(int index = 0, uint32_t defaultValue = 0) const;
   bool getValueAsBoolean(int index = 0, bool defaultValue = false) const;
   
   int getValueCount() const;
   StringList *getValues() const;
};
```

### Logging System

Use debug logging function with tags for better filtering.
New tags should be added to doc/internal/debug_tags.txt

```cpp
// Logging functions
void nxlog_write(int level, const TCHAR *format, ...);
void nxlog_write_tag(int level, const TCHAR *tag, const TCHAR *format, ...);
void nxlog_debug(int level, const TCHAR *format, ...);
void nxlog_debug_tag(const TCHAR *tag, int level, const TCHAR *format, ...);

// Log levels
#define NXLOG_DEBUG     EVENTLOG_DEBUG_TYPE
#define NXLOG_INFO      EVENTLOG_INFORMATION_TYPE  
#define NXLOG_WARNING   EVENTLOG_WARNING_TYPE
#define NXLOG_ERROR     EVENTLOG_ERROR_TYPE

// Setup
bool nxlog_open(const TCHAR *logName, uint32_t flags);
void nxlog_close();
void nxlog_set_debug_level(int level);
```

### Time and Date Utilities

```cpp
// Time conversion
time_t TimeFromString(const TCHAR *str, time_t defaultValue = 0);
void TimeToString(time_t t, TCHAR *buffer);
int64_t GetCurrentTimeMs();  // Milliseconds since epoch
```

## Coding Style and Conventions

### Function Naming
- **PascalCase:** `CreateNewUser`, `ValidatePassword`
- **Descriptive:** Clear, self-documenting names
- **Handler Prefix:** Parameter handlers can use `H_` prefix: `H_FreeDiskSpace`

### Variable Naming
**Deprecated (older code) - Hungarian Notation:**
- `dw` - DWORD (32-bit unsigned)
- `i` - int
- `n` - long
- `qw` - QWORD (64-bit unsigned)  
- `sz` - String array (TCHAR[])
- `psz` - Pointer to string (TCHAR*)
- `p` - Generic pointer
- `tm` - time_t

> Note: Feel free to refactor old code to modern style when touching it.

**Modern (new code) - Avoid Hungarian Notation:**
```cpp
// Preferred in new code
DWORD result = SomeFunction();
int counter = 0;
String userName;

// Avoid in new code  
DWORD dwResult = SomeFunction();
int iCounter = 0;
```

### Code Formatting

#### Indentation and Braces
```cpp
// 3-space indentation, opening brace on new line
if (condition)
{
   // Comment aligned with code
   DWORD result = SomeFunction(param1, param2);
   if (result != 0)
   {
      ProcessResult(result);
   }
}

// Function definition
bool ValidateUser(const TCHAR *userName, const TCHAR *password)
{
   if (userName == nullptr)
      return false;
      
   // Implementation
   return true;
}
```

#### Operators and Spacing
```cpp
// Operators separated by spaces
a = x + 20 / (y - z);
z = function(p1, p2, p3);

// Brackets after control structures (optional spacing)
for(int i = 0; i < count; i++)  // or
for (int i = 0; i < count; i++)
```

#### Comments
```cpp
// Comments describe current state, not history
// Never: "Now we changed this to use X instead of Y"
// Never: "Added function to handle Z"

// Good: Describe purpose and current behavior
// Validate user credentials against LDAP directory
bool result = ValidateLDAPCredentials(user, password);

// Calculate next polling interval based on current load
uint32_t interval = CalculatePollingInterval(nodeLoad);
```

### Data Types

#### Preferred Types
```cpp
// Use NetXMS BYTE typedef 
BYTE    - uint8_t
//Use standard fixed-width types
uint16_t - WORD (legacy) 
uint32_t - DWORD (legacy)
int64_t  - INT64 (legacy)
TCHAR   - Platform-specific character type

// Boolean operations
bool success = true;
BOOL winResult = TRUE;  // legacy code, feel free to refactor to bool
```

#### String Types

Platform support unicode and non unicode builds via TCHAR abstraction for agents. Server side is always unicode so wchar_t and L"text" literals can be used directly.

```cpp
// Platform-specific character handling
#ifdef UNICODE
   typedef wchar_t TCHAR;
   #define _T(x) L##x
#else
   typedef char TCHAR;
   #define _T(x) x
#endif

// Usage
const TCHAR *message = _T("Hello World");
TCHAR buffer[256];
_tcscpy(buffer, message);
```

## Platform Abstractions

NetXMS provides extensive cross-platform abstractions to ensure portability.

### File Operations
```cpp
// Cross-platform file functions
int _access(const char *pathname, int mode);
FILE *_wfopen(const wchar_t *name, const wchar_t *mode);
int _mkdir(const char *pathname, mode_t mode);

// Path separators
#define FS_PATH_SEPARATOR_CHAR  _T('/')   // Unix
#define FS_PATH_SEPARATOR_CHAR  _T('\\')  // Windows
```

### Process Management

Use ProcessExecutor and its ancestors for process creation and management.

```cpp
// Process execution
class LIBNETXMS_EXPORTABLE ProcessExecutor
{
public:
   ProcessExecutor(const TCHAR *cmd, bool shellExec = true, bool selfDestruct = false);
   virtual ~ProcessExecutor();
}

// Process execution with output capture
class LIBNETXMS_EXPORTABLE OutputCapturingProcessExecutor : public ProcessExecutor
{
public:
   OutputCapturingProcessExecutor(const TCHAR* command, bool shellExec = true);
   virtual ~OutputCapturingProcessExecutor();
   
   const String& getOutput() const;
   size_t getOutputSize() const;
   void clearOutput();
};
```

### Byte Order Operations
```cpp
// Network byte order conversion
uint16_t htons(uint16_t hostshort);
uint32_t htonl(uint32_t hostlong);
uint64_t htonq(uint64_t hostquad);

// Custom byte swapping
uint16_t bswap_16(uint16_t val);
uint32_t bswap_32(uint32_t val);
uint64_t bswap_64(uint64_t val);
```

## Best Practices

### Memory Management
1. **Always use NetXMS memory functions** (`MemAlloc`, `MemFree`) instead of `malloc`/`free`
2. **Prefer RAII patterns** with automatic cleanup
3. **Use smart pointers** for complex ownership scenarios
4. **Avoid defense programming** - do not check for null pointers where element should not come as null

### String Handling
1. **Use String class** for dynamic strings
2. **Use TCHAR** for platform-independent character handling
3. **Always check for null** when working with C strings

```cpp
// Good
String userName = GetUserName();
if (!userName.isEmpty())
{
   ProcessUser(userName.cstr());
}

// Avoid
char *userName = GetUserName();
if (userName != NULL && strlen(userName) > 0)  // Manual checks
{
   ProcessUser(userName);
   free(userName);  // Manual memory management
}
```

### Threading
1. **Use NetXMS threading abstractions and thread pools** for portability
2. **Prefer RAII locks** (`LockGuard`) over manual lock/unlock  
4. **Handle thread cleanup** properly

```cpp
// Good
static THREAD_RESULT THREAD_CALL WorkerThread(void *arg)
{
   ThreadSetName("DataProcessor");
   
   LockGuard lock(s_dataMutex);
   // Work with shared data
   
   return THREAD_OK;
}

// Usage
THREAD worker = ThreadCreateEx(WorkerThread, 0, nullptr);
ThreadJoin(worker);
```

### Error Handling
1. **Use consistent return codes** (bool for success/failure, specific codes or enums for details)
2. **Validate input parameters** early
3. **Log errors appropriately** with context

```cpp
bool ProcessUserRequest(const TCHAR *userName, const TCHAR *request)
{
   if (userName == nullptr || request == nullptr)
   {
      nxlog_write(NXLOG_WARNING, _T("ProcessUserRequest: Invalid parameters"));
      return false;
   }
   
   if (!ValidateUser(userName))
   {
      nxlog_write(NXLOG_ERROR, _T("User validation failed for %s"), userName);
      return false;
   }
   
   // Process request
   return true;
}
```

### Container Usage
1. **Use appropriate containers** for the task (Array for sequences, HashMap for lookups)
2. **Consider ownership** when adding objects to containers

### Debugging and Logging
1. **Use tagged debug output** for filtering
2. **Include context** in log messages
3. **Use appropriate log levels**

```cpp
// Tagged debug output
nxlog_debug_tag(_T("db.query"), 6, _T("Executing SQL: %s"), query);

// Contextual logging  
nxlog_write(NXLOG_INFO, _T("Node %s [%u] status changed to %s"), 
           nodeName, nodeId, StatusToString(newStatus));
```

## Server-Side Data Structures

Understanding NetXMS server-side data structures is crucial for developing server components, plugins, and extensions.

### Core Object Hierarchy

NetXMS uses an object-oriented approach where all manageable entities inherit from a base `NetObj` class.

#### Base NetObj Class

**Location:** `src/server/include/nms_objects.h:1269`

```cpp
class NXCORE_EXPORTABLE NetObj : public NObject
{
protected:
   uint32_t m_id;                    // Unique object ID
   uuid m_guid;                      // Globally unique identifier
   TCHAR m_name[MAX_OBJECT_NAME];    // Object name
   int m_status;                     // Current object status
   int m_savedStatus;                // Status in database
   uint32_t m_flags;                 // Object flags
   uint32_t m_runtimeFlags;          // Runtime flags
   time_t m_timestamp;               // Last change timestamp
   GeoLocation m_geoLocation;        // Geographic location
   AccessList m_accessList;          // Access control list
   Mutex m_mutexProperties;          // Property protection
   
public:
   // Status constants
   static const int STATUS_NORMAL = 0;
   static const int STATUS_WARNING = 1;
   static const int STATUS_MINOR = 2;
   static const int STATUS_MAJOR = 3;
   static const int STATUS_CRITICAL = 4;
   static const int STATUS_UNKNOWN = 5;
   static const int STATUS_UNMANAGED = 6;
   static const int STATUS_DISABLED = 7;
   static const int STATUS_TESTING = 8;
};
```

#### Primary Object Types

**Node Class** (`nms_objects.h:3443`)
```cpp
class NXCORE_EXPORTABLE Node : public DataCollectionTarget
{
protected:
   InetAddress m_primaryIpAddr;      // Primary IP address
   TCHAR m_primaryName[MAX_DNS_NAME]; // Primary hostname
   uint32_t m_capabilities;          // Node capabilities
   uint32_t m_platformName;          // Platform name ID
   TCHAR m_snmpObjectId[MAX_OID_LEN]; // SNMP object ID
   AgentConnectionEx *m_agentConnection; // Agent connection
   SNMP_Transport *m_snmpTransport;  // SNMP transport
   
public:
   bool connectToAgent();
   bool callAgent(const TCHAR *command, StringList *args, AbstractCommSession *session);
   DataCollectionError getItemFromAgent(const TCHAR *param, TCHAR *buffer);
   DataCollectionError getTableFromAgent(const TCHAR *param, Table **table);
};
```

**Interface Class** (`nms_objects.h:2189`)
```cpp
class NXCORE_EXPORTABLE Interface : public NetObj
{
protected:
   uint32_t m_type;                  // Interface type
   uint32_t m_index;                 // Interface index
   MacAddress m_macAddr;             // MAC address
   InetAddressList m_ipAddressList;  // IP addresses
   uint32_t m_bridgePortNumber;      // Bridge port number
   uint32_t m_slot;                  // Physical slot
   uint32_t m_port;                  // Physical port
   
public:
   bool isPhysicalPort() const;
   bool isLoopback() const;
   const MacAddress& getMacAddr() const { return m_macAddr; }
   const InetAddressList *getIpAddressList() const { return &m_ipAddressList; }
};
```

**DataCollectionTarget** (`nms_objects.h:1902`)
```cpp
class NXCORE_EXPORTABLE DataCollectionTarget : public NetObj
{
protected:
   ObjectArray<DCObject> m_dcObjects; // Data collection items
   Mutex m_hPollerMutex;             // Poller synchronization
   time_t m_lastConfigurationPoll;   // Last config poll time
   time_t m_lastStatusPoll;          // Last status poll time
   time_t m_lastDataPoll;            // Last data poll time
   
public:
   void addDCObject(DCObject *object);
   void deleteDCObject(uint32_t dcObjectId);
   DCObject *getDCObjectById(uint32_t id) const;
   void scheduleItemDataCollection(uint32_t dcObjectId);
};
```

### Alarm Management

#### Alarm Class

**Location:** `src/server/include/nms_alarm.h:34`

```cpp
class NXCORE_EXPORTABLE Alarm
{
protected:
   uint64_t m_sourceEventId;         // Originating event ID
   uint32_t m_alarmId;               // Unique alarm ID
   uint32_t m_parentAlarmId;         // Parent alarm for correlation
   time_t m_creationTime;            // Creation timestamp
   time_t m_lastChangeTime;          // Last modification
   uint32_t m_sourceObject;          // Source object ID
   uint32_t m_sourceEventCode;       // Triggering event code
   BYTE m_currentSeverity;           // Current severity (0-4)
   BYTE m_state;                     // Alarm state
   TCHAR m_message[MAX_EVENT_MSG_LENGTH]; // Alarm message
   TCHAR m_key[MAX_DB_STRING];       // Unique correlation key
   
public:
   // Alarm states
   enum State
   {
      OUTSTANDING = 0,
      ACKNOWLEDGED = 1,
      RESOLVED = 2,
      TERMINATED = 3
   };
   
   void acknowledge(uint32_t userId, bool sticky, uint32_t timeout);
   void resolve(uint32_t userId, Event *event, bool terminate);
   void terminate(uint32_t userId);
};
```

#### Alarm Lifecycle
1. **Creation** - Generated by Event Processing Policy (EPP) from events
2. **Acknowledgment** - User acknowledges awareness of the problem
3. **Resolution** - Problem is fixed (alarm can be terminated)
4. **Termination** - Alarm is closed and moved to history

### Data Collection Items (DCIs)

#### DCObject Base Class

**Location:** `src/server/include/nms_dcoll.h:280`

```cpp
class NXCORE_EXPORTABLE DCObject
{
protected:
   uint32_t m_id;                    // Unique DCI ID
   uint32_t m_ownerId;               // Owner object ID
   SharedString m_name;              // DCI name/parameter
   SharedString m_description;       // Human-readable description
   time_t m_lastPoll;                // Last collection time
   int32_t m_pollingInterval;        // Collection frequency (seconds)
   int32_t m_retentionTime;          // Data retention (days)
   BYTE m_source;                    // Data source type
   BYTE m_status;                    // DCI status
   ObjectArray<DCIThreshold> m_thresholds; // Threshold definitions
   
public:
   // Data sources
   enum DataSource
   {
      DS_NATIVE_AGENT = 0,
      DS_SNMP_AGENT = 1,
      DS_WEB_SERVICE = 2,
      DS_PUSH_AGENT = 3,
      DS_WINPERF = 4,
      DS_INTERNAL = 5,
      DS_SSH = 6,
      DS_MQTT = 7,
      DS_DEVICE_DRIVER = 8
   };
   
   virtual DataCollectionError collect(DataCollectionTarget *target) = 0;
   virtual void processNewValue(time_t timestamp, const void *value) = 0;
};
```

#### DCItem and DCTable

```cpp
class NXCORE_EXPORTABLE DCItem : public DCObject
{
protected:
   BYTE m_dataType;                  // Data type (int, float, string, etc.)
   TCHAR m_delta;                    // Delta calculation mode
   int32_t m_sampleCount;            // Samples for averaging
   
public:
   void updateFromTemplate(DCItem *templateItem);
   ItemValue *getInternalValue() const;
   DataCollectionError getValueFromAgent(DataCollectionTarget *target);
};

class NXCORE_EXPORTABLE DCTable : public DCObject
{
protected:
   ObjectArray<DCTableColumn> m_columns; // Table columns
   
public:
   void updateFromTemplate(DCTable *templateTable);
   Table *getLastValue() const;
   DataCollectionError getTableFromAgent(DataCollectionTarget *target);
};
```

### Event System

#### Event Class

**Location:** `src/server/include/nms_events.h:133`

```cpp
class NXCORE_EXPORTABLE Event
{
protected:
   uint64_t m_id;                    // Unique event ID
   uint32_t m_code;                  // Event code (type)
   int m_severity;                   // Severity level
   uint32_t m_sourceId;              // Source object ID
   time_t m_timestamp;               // Event timestamp
   TCHAR m_name[MAX_EVENT_NAME];     // Event name
   TCHAR *m_messageText;             // Formatted message
   StringList m_parameters;          // Event parameters
   
public:
   // Severity levels
   enum Severity
   {
      SEV_NORMAL = 0,
      SEV_WARNING = 1,
      SEV_MINOR = 2,
      SEV_MAJOR = 3,
      SEV_CRITICAL = 4
   };
   
   static shared_ptr<Event> createFromTemplate(EventTemplate *tmpl, NetObj *source);
   void expandMessageText();
   const TCHAR *getSeverityName() const;
};
```

### Object Relationships and Management

#### Object Index System
```cpp
// Global object index - thread-safe shared pointer management
extern SharedPointerIndex<NetObj> g_objectIndex;

// Finding objects
shared_ptr<NetObj> FindObjectById(uint32_t id);
shared_ptr<Node> FindNodeById(uint32_t id);
shared_ptr<Interface> FindInterfaceById(uint32_t id);

// Creating objects
shared_ptr<Node> CreateNode(const TCHAR *name, const InetAddress& addr);
void RegisterNetworkObject(const shared_ptr<NetObj>& object);
```

#### Parent-Child Relationships
```cpp
// Link objects in hierarchy
void NetObj::linkObjects(const shared_ptr<NetObj>& parent, const shared_ptr<NetObj>& child);

// Access children and parents
ObjectArray<NetObj> *getChildren(int typeFilter = -1);
ObjectArray<NetObj> *getParents(int typeFilter = -1);
bool isChild(uint32_t id);
bool isDirectChild(uint32_t id);
```

### Database Schema Integration

When database change is needed please write an upgrade procedure in `src/server/tools/nxdbmgr` and change schema for correct version and init data in `sql/` folder.

Please refer to [data dictionary](https://www.netxms.org/documentation/datadictionary-latest/) for detailed information about database structure.

#### Key Tables
- **objects** - Base object properties (id, name, status, etc.)
- **nodes** - Node-specific data (IP addresses, capabilities)
- **interfaces** - Interface information (MAC, IP, type)
- **items** - Data collection item definitions
- **idata_xx** - Time series data storage (one table per data type)
- **thresholds** - Threshold definitions for DCIs
- **event_log** - Event history
- **alarms** - Active and historical alarms

#### Relationships
```sql
-- Objects are the base for all entities
objects.id ← nodes.id (1:1)
objects.id ← interfaces.id (1:1)
objects.id ← subnets.id (1:1)

-- Data collection relationships
items.node_id → objects.id (N:1)
thresholds.item_id → items.item_id (N:1)

-- Event and alarm relationships
event_log.event_source → objects.id (N:1)
alarms.source_object_id → objects.id (N:1)
alarms.source_event_id → event_log.event_id (1:1)
```

### Thread Safety and Concurrency

#### Object Protection
```cpp
class NetObj
{
protected:
   Mutex m_mutexProperties;          // Protects object properties
   Mutex m_mutexACL;                 // Protects access control list
   VolatileCounter m_modified;       // Modification tracking
   
public:
   void lockProperties() { m_mutexProperties.lock(); }
   void unlockProperties() { m_mutexProperties.unlock(); }
   bool isModified() const { return m_modified.get() != 0; }
   void setModified(uint32_t flags = MODIFY_ALL);
};
```

#### Shared Pointer Management
```cpp
// Objects managed via shared_ptr for memory safety
typedef shared_ptr<NetObj> NetObjPtr;
typedef shared_ptr<Node> NodePtr;
typedef shared_ptr<Interface> InterfacePtr;

// Thread-safe object storage
template<class T>
class SharedPointerIndex
{
public:
   shared_ptr<T> get(uint32_t id);
   void put(uint32_t id, const shared_ptr<T>& object);
   void remove(uint32_t id);
   void forEach(std::function<void (const shared_ptr<T>&)> callback);
};
```

### Best Practices for Server Development

1. **Use shared_ptr** for object references
2. **Lock object properties** before modification
3. **Use object IDs** for persistent references
4. **Handle object lifecycle** properly (creation, modification, deletion)
5. **Understand status propagation** in object hierarchy
6. **Use events** for triggering automation
7. **Design for scalability** - consider distributed deployments

This understanding of NetXMS server data structures provides the foundation for developing server components, understanding object relationships, and implementing custom functionality within the NetXMS architecture.

---

This guide provides the foundation for C++ development in NetXMS. The key is to understand and consistently use the NetXMS abstractions and patterns rather than mixing different approaches within the same codebase.
