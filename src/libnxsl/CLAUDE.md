# NXSL Scripting Language (libnxsl)

> Shared guidelines: see [root CLAUDE.md](../../CLAUDE.md).

## Object Arguments

When a method accepts an argument that may be either an NXSL object or an integer ID, cast the object's data straight to the `shared_ptr` — don't extract the ID and re-resolve it via `FindObjectById`. You already hold a valid reference, and the round-trip risks a spurious `nullptr` if the object becomes unreachable by ID while the script still holds it. Use `FindObjectById` only for the integer-ID branch:

```cpp
shared_ptr<Interface> peer;
if (argv[0]->isObject(L"Interface"))
   peer = *static_cast<shared_ptr<Interface>*>(argv[0]->getValueAsObject()->getData());
else if (argv[0]->isInteger())
   peer = static_pointer_cast<Interface>(FindObjectById(argv[0]->getValueAsUInt32(), OBJECT_INTERFACE));
```

## Value Truthiness

`NXSL_Value::isFalse()` returns `true` for both `false` and `null`. To distinguish an explicit `false` from "no return value", check `isNull()` first.

## Testing Scripts

`nxscript <file>.nxsl` (built from `src/nxscript/`) runs a script outside the server.
