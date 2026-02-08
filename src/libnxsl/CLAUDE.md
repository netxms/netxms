# NXSL Scripting Language (libnxsl)

> Shared guidelines: See [root CLAUDE.md](../../CLAUDE.md) for C++ development guidelines.

## Overview

NXSL (NetXMS Scripting Language) is a C-like scripting language used throughout NetXMS for:
- Transformation scripts (DCI values)
- Filter scripts (event processing)
- Action scripts (automated responses)
- Instance discovery scripts
- Object scripts

## Directory Structure

```
src/libnxsl/
├── parser.y        # Bison grammar file
├── parser.l        # Flex lexer file
├── compiler.cpp    # Script compiler
├── program.cpp     # Compiled program representation
├── vm.cpp          # Virtual machine (main interpreter)
├── env.cpp         # Script environment
├── functions.cpp   # Built-in functions
├── class.cpp       # Class support
├── value.cpp       # Value handling
├── variable.cpp    # Variable handling
├── instruction.cpp # VM instructions
├── library.cpp     # Library management
└── ...             # Type-specific files
```

## Key Components

### Parser/Compiler

| File | Description |
|------|-------------|
| `parser.y` | Bison grammar definition |
| `parser.l` | Flex lexer definition |
| `compiler.cpp` | Compiles AST to bytecode |
| `program.cpp` | Compiled program representation |

### Virtual Machine

| File | Description |
|------|-------------|
| `vm.cpp` | Main VM execution loop (100K+ lines) |
| `instruction.cpp` | VM instruction implementation |
| `env.cpp` | Script execution environment |

### Data Types

| File | Class/Type | Description |
|------|------------|-------------|
| `value.cpp` | `NXSL_Value` | Value container |
| `array.cpp` | `NXSL_Array` | Array type |
| `hashmap.cpp` | `NXSL_HashMap` | Hash map type |
| `string.cpp` | String methods | String operations |
| `table.cpp` | `NXSL_Table` | Table type |
| `inetaddr.cpp` | `NXSL_InetAddress` | IP address type |
| `macaddr.cpp` | `NXSL_MacAddress` | MAC address type |
| `geolocation.cpp` | `NXSL_GeoLocation` | Geolocation type |
| `bytestream.cpp` | `NXSL_ByteStream` | Binary data type |
| `json.cpp` | JSON support | JSON parsing/generation |
| `time.cpp` | Time functions | Date/time operations |

### Built-in Functions

| File | Description |
|------|-------------|
| `functions.cpp` | Core built-in functions |
| `io.cpp` | I/O functions (file operations) |
| `math.cpp` | Mathematical functions |

## Adding Built-in Functions

### Simple Function

```cpp
// In functions.cpp or appropriate file

/**
 * my_function(arg1, arg2) - Description
 */
static int F_my_function(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   // Validate argument count
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   // Get arguments
   const TCHAR *str = argv[0]->getValueAsCString();
   int num = argv[1]->getValueAsInt32();

   // Return result
   *result = vm->createValue(_T("result"));
   return 0;  // Success
}

// Register in s_nxslStdFunctions array:
{ _T("my_function"), F_my_function, 2 },  // name, function, arg count (-1 for variable)
```

### Function with Variable Arguments

```cpp
static int F_my_vararg_function(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (argc < 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   // Process variable arguments
   for (int i = 0; i < argc; i++)
   {
      // Process argv[i]
   }

   *result = vm->createValue(42);
   return 0;
}

// Register with -1 for arg count:
{ _T("my_vararg"), F_my_vararg_function, -1 },
```

## Adding NXSL Classes

### Class Definition

```cpp
// In class.cpp or new file

class NXSL_MyClass : public NXSL_Class
{
public:
   NXSL_MyClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const NXSL_Identifier& attr) override;
   virtual bool setAttr(NXSL_Object *object, const NXSL_Identifier& attr, NXSL_Value *value) override;
};

NXSL_MyClass::NXSL_MyClass() : NXSL_Class()
{
   setName(_T("MyClass"));
}

NXSL_Value *NXSL_MyClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   MyData *data = static_cast<MyData*>(object->getData());

   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   if (NXSL_COMPARE_ATTRIBUTE_NAME("myAttr"))
   {
      return object->vm()->createValue(data->myValue);
   }

   return nullptr;
}
```

### Class Method

```cpp
// Add to class definition
NXSL_METHOD_DEFINITION(MyClass, myMethod)
{
   MyData *data = static_cast<MyData*>(object->getData());

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   // Do something with data
   *result = object->vm()->createValue(true);
   return 0;
}

// Register method in constructor:
NXSL_MyClass::NXSL_MyClass()
{
   setName(_T("MyClass"));
   NXSL_REGISTER_METHOD(MyClass, myMethod, 1);  // name, arg count
}
```

## Error Codes

| Code | Meaning |
|------|---------|
| `NXSL_ERR_SUCCESS` (0) | Success |
| `NXSL_ERR_NOT_STRING` | Argument is not a string |
| `NXSL_ERR_NOT_INTEGER` | Argument is not an integer |
| `NXSL_ERR_NOT_NUMBER` | Argument is not a number |
| `NXSL_ERR_NOT_ARRAY` | Argument is not an array |
| `NXSL_ERR_NOT_HASHMAP` | Argument is not a hash map |
| `NXSL_ERR_NOT_OBJECT` | Argument is not an object |
| `NXSL_ERR_INVALID_ARGUMENT_COUNT` | Wrong number of arguments |
| `NXSL_ERR_BAD_CONDITION` | Invalid condition |
| `NXSL_ERR_INTERNAL` | Internal error |

## Script Environment

Scripts run in an environment that provides:
- Global variables
- Constants
- Functions
- Objects

```cpp
// Create environment
NXSL_Environment *env = new NXSL_Environment();
env->registerIOFunctions();  // Enable I/O
env->setGlobalVariable(_T("myVar"), vm->createValue(42));

// Compile and run
NXSL_Program *program = NXSLCompile(script, errorMessage, maxErrorLen);
NXSL_VM *vm = new NXSL_VM(env);
vm->load(program);
vm->run();
```

## Server-Specific Extensions

The server adds additional NXSL classes and functions in `src/server/core/`:
- Node, Interface, Zone, etc.
- Database functions
- Alarm/Event functions
- Action functions

## Debugging

```bash
# Use nxscript tool to test scripts
/opt/netxms/bin/nxscript test.nxsl

# Debug tags
nxlog_debug_tag(_T("nxsl"), level, ...)      # General NXSL
nxlog_debug_tag(_T("nxsl.vm"), level, ...)   # Virtual machine
```

## Documentation

- [NXSL Language Reference](https://www.netxms.org/documentation/nxsl-latest/)
- Syntax highlighting files: `doc/nxsl.npp.*.xml`

## Related Components

- [libnetxms](../libnetxms/CLAUDE.md) - Core library
- [Server](../server/CLAUDE.md) - Server-specific NXSL extensions
- [Agent](../agent/CLAUDE.md) - Uses NXSL for action scripts
