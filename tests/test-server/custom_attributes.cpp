/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: custom_attributes.cpp
**
** Tests for structured (JSON) custom attribute values: server API, NXSL access,
** inheritance, and import from client messages.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <testtools.h>

/**
 * Create container object with given name and insert it into object indexes
 */
static shared_ptr<Container> CreateContainer(const wchar_t *name)
{
   shared_ptr<Container> object = make_shared<Container>();
   object->setName(name);
   NetObjInsert(object, true, false);
   return object;
}

/**
 * Get flags of custom attribute (0 if attribute does not exist)
 */
static uint32_t GetAttributeFlags(const NetObj& object, const wchar_t *name)
{
   uint32_t flags = 0;
   object.forEachCustomAttribute(
      [name, &flags] (const wchar_t *n, const CustomAttribute *attr) -> EnumerationCallbackResult
      {
         if (!wcscmp(n, name))
         {
            flags = attr->flags;
            return _STOP;
         }
         return _CONTINUE;
      });
   return flags;
}

/**
 * Compile script in server environment and create VM. Compilation errors fail the test.
 */
static NXSL_VM *CompileScript(const wchar_t *source)
{
   NXSL_CompilationDiagnostic diag;
   NXSL_VM *vm = NXSLCompileAndCreateVM(source, new NXSL_ServerEnv(), &diag);
   if (vm == nullptr)
      WriteToTerminalEx(L"\n   Compilation error: %s\n", diag.errorText.cstr());
   AssertNotNull(vm);
   return vm;
}

/**
 * Compile and run script in server environment. Script errors and failed assertions fail the test.
 */
static void RunScript(const wchar_t *source)
{
   NXSL_VM *vm = CompileScript(source);
   bool success = vm->run();
   if (!success)
      WriteToTerminalEx(L"\n   Script error: %s\n   Assertion message: %s\n", vm->getErrorText(), vm->getAssertMessage());
   AssertTrue(success);
   delete vm;
}

/**
 * Compile and run script that is expected to fail at runtime with given error code
 */
static void RunScriptExpectError(const wchar_t *source, int expectedError)
{
   NXSL_VM *vm = CompileScript(source);
   AssertFalse(vm->run());
   AssertEquals(vm->getErrorCode(), expectedError);
   delete vm;
}

/**
 * Structured values set and read through server API
 */
static void TestServerApi()
{
   StartTest(L"Custom attributes: structured values via server API");

   shared_ptr<Container> object = CreateContainer(L"ca-api");

   json_t *json = json_object();
   json_object_set_new(json, "zeta", json_integer(1));
   json_object_set_new(json, "alpha", json_string("x y"));
   object->setCustomAttribute(L"s", json, StateChange::IGNORE);
   json_decref(json);

   AssertEquals(GetAttributeFlags(*object, L"s"), static_cast<uint32_t>(CAF_JSON));
   AssertTrue(!wcscmp(object->getCustomAttribute(L"s").cstr(), L"{\"alpha\":\"x y\",\"zeta\":1}"));   // Canonical form: compact, sorted keys

   json_t *attributes = object->getCustomAttributesAsJson();
   json_t *value = json_object_get(attributes, "s");
   AssertTrue(json_is_object(value));
   AssertEquals(static_cast<int64_t>(json_integer_value(json_object_get(value, "zeta"))), static_cast<int64_t>(1));
   json_decref(attributes);

   // Plain string write clears the flag and keeps text as is
   object->setCustomAttribute(L"s", L"{\"b\":2, \"a\":1}");
   AssertEquals(GetAttributeFlags(*object, L"s"), 0u);
   AssertTrue(!wcscmp(object->getCustomAttribute(L"s").cstr(), L"{\"b\":2, \"a\":1}"));

   attributes = object->getCustomAttributesAsJson();
   AssertTrue(json_is_string(json_object_get(attributes, "s")));
   json_decref(attributes);

   EndTest();
}

/**
 * Structured values in NXSL
 */
static void TestNXSL()
{
   StartTest(L"Custom attributes: structured values in NXSL");

   shared_ptr<Container> object = CreateContainer(L"ca-nxsl");

   RunScript(LR"NXSL(
      obj = FindObject("ca-nxsl");
      assert(obj != null, "object lookup");

      // Hash map round trip
      prev = obj->setCustomAttribute("lowDisk", { "count": 3, "last": 1700000000, "tags": ["a", "b"], "nested": { "ok": true, "none": null } });
      assert(prev == null, "no previous value");
      v = obj->getCustomAttribute("lowDisk");
      assert(typeof(v) == "hashmap", "hash map returned");
      assert(v["count"] == 3, "integer leaf");
      assert(v["last"] == 1700000000, "large integer leaf");
      assert(typeof(v["tags"]) == "array", "nested array");
      assert(v["tags"][1] == "b", "nested array element");
      assert(typeof(v["nested"]) == "hashmap", "nested hash map");
      assert(v["nested"]["ok"] == true, "boolean leaf");
      assert(v["nested"]["none"] == null, "null leaf");

      // Read-modify-write
      v["count"]++;
      obj->setCustomAttribute("lowDisk", v);
      v = obj->getCustomAttribute("lowDisk");
      assert(v["count"] == 4, "read-modify-write");

      // Attribute and customAttributes access paths
      v = obj.lowDisk;
      assert(typeof(v) == "hashmap", "attribute access");
      assert(v["count"] == 4, "attribute access value");
      all = obj.customAttributes;
      v = all["lowDisk"];
      assert(typeof(v) == "hashmap", "customAttributes access");
      assert(v["count"] == 4, "customAttributes access value");

      // Array round trip
      obj->setCustomAttribute("list", [1, "two", 3.5]);
      l = obj->getCustomAttribute("list");
      assert(typeof(l) == "array", "array returned");
      assert(l.size == 3, "array size");
      assert(l[1] == "two", "array string element");
      assert(l[2] == 3.5, "array real element");

      // JsonObject in, hash map out
      j = new JsonObject();
      j->set("x", 1);
      obj->setCustomAttribute("json", j);
      v = obj->getCustomAttribute("json");
      assert(typeof(v) == "hashmap", "JsonObject stored as structured value");
      assert(v["x"] == 1, "JsonObject member");

      // Previous value returned by setCustomAttribute and deleteCustomAttribute is native
      prev = obj->setCustomAttribute("list", "plain");
      assert(typeof(prev) == "array", "previous structured value");
      assert(obj->getCustomAttribute("list") == "plain", "plain string replaces structured value");
      prev = obj->deleteCustomAttribute("json");
      assert(typeof(prev) == "hashmap", "deleted structured value");
      assert(obj->getCustomAttribute("json") == null, "deleted");

      // Legacy global functions
      SetCustomAttribute(obj, "legacy", { "k": "v" });
      v = GetCustomAttribute(obj, "legacy");
      assert(v["k"] == "v", "legacy functions");

      // Scalars are stored as text without the flag
      obj->setCustomAttribute("num", 42);
      assert(obj->getCustomAttribute("num") == 42, "numeric value");
      obj->setCustomAttribute("flag", true);
      assert(obj->getCustomAttribute("flag") == true, "boolean value");
   )NXSL");

   AssertEquals(GetAttributeFlags(*object, L"lowDisk"), static_cast<uint32_t>(CAF_JSON));
   AssertEquals(GetAttributeFlags(*object, L"legacy"), static_cast<uint32_t>(CAF_JSON));
   AssertEquals(GetAttributeFlags(*object, L"list"), 0u);
   AssertEquals(GetAttributeFlags(*object, L"num"), 0u);
   AssertTrue(!wcscmp(object->getCustomAttribute(L"num").cstr(), L"42"));
   AssertTrue(!wcscmp(object->getCustomAttribute(L"legacy").cstr(), L"{\"k\":\"v\"}"));

   // Objects other than JsonObject and JsonArray are rejected
   RunScriptExpectError(L"obj = FindObject(\"ca-nxsl\"); obj->setCustomAttribute(\"bad\", obj);", NXSL_ERR_NOT_CONTAINER);
   // Null is rejected
   RunScriptExpectError(L"obj = FindObject(\"ca-nxsl\"); obj->setCustomAttribute(\"bad\", null);", NXSL_ERR_NOT_STRING);
   AssertEquals(GetAttributeFlags(*object, L"bad"), 0u);

   EndTest();
}

/**
 * Structured value inheritance: type flag travels with the value
 */
static void TestInheritance()
{
   StartTest(L"Custom attributes: structured value inheritance");

   shared_ptr<Container> parent = CreateContainer(L"ca-parent");
   shared_ptr<Container> child = CreateContainer(L"ca-child");
   shared_ptr<Container> grandchild = CreateContainer(L"ca-grandchild");

   json_t *json = json_array();
   json_array_append_new(json, json_integer(1));
   json_array_append_new(json, json_integer(2));
   parent->setCustomAttribute(L"list", json, StateChange::SET);

   // Linking propagates value together with type flag
   NetObj::linkObjects(parent, child);
   NetObj::linkObjects(child, grandchild);
   AssertEquals(GetAttributeFlags(*child, L"list"), static_cast<uint32_t>(CAF_INHERITABLE | CAF_JSON));
   AssertEquals(GetAttributeFlags(*grandchild, L"list"), static_cast<uint32_t>(CAF_INHERITABLE | CAF_JSON));
   AssertTrue(!wcscmp(grandchild->getCustomAttribute(L"list").cstr(), L"[1,2]"));

   // Change of parent value to plain text with the same content clears flag on inherited copies
   parent->setCustomAttribute(L"list", L"[1,2]", StateChange::SET);
   AssertEquals(GetAttributeFlags(*parent, L"list"), static_cast<uint32_t>(CAF_INHERITABLE));
   AssertEquals(GetAttributeFlags(*child, L"list"), static_cast<uint32_t>(CAF_INHERITABLE));
   AssertEquals(GetAttributeFlags(*grandchild, L"list"), static_cast<uint32_t>(CAF_INHERITABLE));

   // Change back to structured value with the same text sets flag on inherited copies
   parent->setCustomAttribute(L"list", json, StateChange::SET);
   json_decref(json);
   AssertEquals(GetAttributeFlags(*child, L"list"), static_cast<uint32_t>(CAF_INHERITABLE | CAF_JSON));
   AssertEquals(GetAttributeFlags(*grandchild, L"list"), static_cast<uint32_t>(CAF_INHERITABLE | CAF_JSON));

   // Redefinition on child with plain value; grandchild follows child
   child->setCustomAttribute(L"list", L"redefined", StateChange::SET);
   AssertEquals(GetAttributeFlags(*child, L"list"), static_cast<uint32_t>(CAF_INHERITABLE | CAF_REDEFINED));
   AssertEquals(GetAttributeFlags(*grandchild, L"list"), static_cast<uint32_t>(CAF_INHERITABLE));
   AssertTrue(!wcscmp(grandchild->getCustomAttribute(L"list").cstr(), L"redefined"));

   // Removing redefinition restores structured value from parent
   child->deleteCustomAttribute(L"list");
   AssertEquals(GetAttributeFlags(*child, L"list"), static_cast<uint32_t>(CAF_INHERITABLE | CAF_JSON));
   AssertEquals(GetAttributeFlags(*grandchild, L"list"), static_cast<uint32_t>(CAF_INHERITABLE | CAF_JSON));
   AssertTrue(!wcscmp(child->getCustomAttribute(L"list").cstr(), L"[1,2]"));
   AssertTrue(!wcscmp(grandchild->getCustomAttribute(L"list").cstr(), L"[1,2]"));

   EndTest();
}

/**
 * Structured values imported from client message
 */
static void TestMessageImport()
{
   StartTest(L"Custom attributes: structured values from client message");

   shared_ptr<Container> object = CreateContainer(L"ca-msg");

   NXCPMessage msg(CMD_MODIFY_OBJECT, 0);
   msg.setField(VID_NUM_CUSTOM_ATTRIBUTES, static_cast<uint32_t>(2));
   msg.setField(VID_CUSTOM_ATTRIBUTES_BASE, L"structured");
   msg.setField(VID_CUSTOM_ATTRIBUTES_BASE + 1, L"{ \"b\" : 2, \"a\" : [1, 2] }");
   msg.setField(VID_CUSTOM_ATTRIBUTES_BASE + 2, static_cast<uint32_t>(CAF_JSON));
   msg.setField(VID_CUSTOM_ATTRIBUTES_BASE + 3, L"plain");
   msg.setField(VID_CUSTOM_ATTRIBUTES_BASE + 4, L"[1, 2]");
   msg.setField(VID_CUSTOM_ATTRIBUTES_BASE + 5, static_cast<uint32_t>(CAF_INHERITABLE));
   AssertEquals(object->setCustomAttributesFromMessage(msg), RCC_SUCCESS);

   AssertEquals(GetAttributeFlags(*object, L"structured"), static_cast<uint32_t>(CAF_JSON));
   AssertTrue(!wcscmp(object->getCustomAttribute(L"structured").cstr(), L"{\"a\":[1,2],\"b\":2}"));   // Canonical form
   AssertEquals(GetAttributeFlags(*object, L"plain"), static_cast<uint32_t>(CAF_INHERITABLE));
   AssertTrue(!wcscmp(object->getCustomAttribute(L"plain").cstr(), L"[1, 2]"));

   // Value marked as structured must be a JSON object or array; existing attributes are not touched on error
   NXCPMessage badMessage(CMD_MODIFY_OBJECT, 0);
   badMessage.setField(VID_NUM_CUSTOM_ATTRIBUTES, static_cast<uint32_t>(1));
   badMessage.setField(VID_CUSTOM_ATTRIBUTES_BASE, L"bad");
   badMessage.setField(VID_CUSTOM_ATTRIBUTES_BASE + 1, L"42");
   badMessage.setField(VID_CUSTOM_ATTRIBUTES_BASE + 2, static_cast<uint32_t>(CAF_JSON));
   AssertEquals(object->setCustomAttributesFromMessage(badMessage), RCC_INVALID_ARGUMENT);
   AssertEquals(GetAttributeFlags(*object, L"bad"), 0u);
   AssertEquals(GetAttributeFlags(*object, L"structured"), static_cast<uint32_t>(CAF_JSON));

   EndTest();
}

/**
 * Entry point for custom attribute tests
 */
void TestCustomAttributes()
{
   TestServerApi();
   TestNXSL();
   TestInheritance();
   TestMessageImport();
}
