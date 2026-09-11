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
** File: test-physical-placement.cpp
**
** Unit tests for the "physicalPlacement" property group: serialization, merge-patch
** parsing with strict type validation, rack extent geometry, and chassis placement XML
** serialization. Compiles the real src/server/core/physical_placement.cpp.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <testtools.h>

/**
 * Stub for the server object index. Constructing a real Rack or Chassis would pull in the
 * whole server, so container lookup always misses here: the tests cover every path except
 * the three that need a live container object (rack extent, chassis class acceptance and
 * the descendant loop check), and IsValidRackExtent is tested directly instead.
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectById(uint32_t id, int objectClassHint)
{
   return shared_ptr<NetObj>();
}

/**
 * Placement state under test, together with the reference structure addressing it
 */
struct TestPlacement
{
   uint32_t containerId;
   int16_t position;
   int16_t height;
   RackOrientation orientation;
   uuid imageFront;
   uuid imageRear;
   PhysicalPlacementRef ref;

   TestPlacement()
   {
      containerId = 0;
      position = 0;
      height = 1;
      orientation = FILL;
      imageFront = uuid::NULL_UUID;
      imageRear = uuid::NULL_UUID;
      ref.containerId = &containerId;
      ref.position = &position;
      ref.height = &height;
      ref.orientation = &orientation;
      ref.imageFront = &imageFront;
      ref.imageRear = &imageRear;
   }
};

/**
 * Serialization must emit every key of the group
 */
static void TestPlacementToJson()
{
   StartTest(_T("Physical placement serialization"));

   TestPlacement p;
   p.containerId = 326;
   p.position = 36;
   p.height = 2;
   p.orientation = REAR;
   p.imageFront = uuid::parseA("fd21a79f-e8d4-4c28-bf07-81b5fce4a26a");

   json_t *group = PhysicalPlacementToJson(p.ref);
   AssertNotNull(group);
   AssertEquals(json_object_get_uint32(group, "containerId"), static_cast<uint32_t>(326));
   AssertEquals(json_object_get_int32(group, "position"), 36);
   AssertEquals(json_object_get_int32(group, "height"), 2);
   AssertEquals(json_object_get_int32(group, "orientation"), 2);
   AssertEquals(json_object_get_string_utf8(group, "imageFront", ""), "fd21a79f-e8d4-4c28-bf07-81b5fce4a26a");
   AssertEquals(json_object_get_string_utf8(group, "imageRear", ""), "00000000-0000-0000-0000-000000000000");
   json_decref(group);

   EndTest();
}

/**
 * Rack extent geometry, both numbering directions
 */
static void TestRackExtent()
{
   StartTest(_T("Rack extent validation"));

   // Top-bottom numbering: position is the device's topmost unit
   AssertTrue(IsValidRackExtent(1, 1, 42, true));
   AssertTrue(IsValidRackExtent(42, 1, 42, true));
   AssertTrue(IsValidRackExtent(41, 2, 42, true));
   AssertFalse(IsValidRackExtent(42, 2, 42, true));
   AssertFalse(IsValidRackExtent(43, 1, 42, true));
   AssertFalse(IsValidRackExtent(0, 1, 42, true));

   // Bottom-top numbering: position is the device's bottommost unit
   AssertTrue(IsValidRackExtent(1, 1, 42, false));
   AssertTrue(IsValidRackExtent(42, 1, 42, false));
   AssertTrue(IsValidRackExtent(2, 2, 42, false));
   AssertFalse(IsValidRackExtent(1, 2, 42, false));
   AssertFalse(IsValidRackExtent(43, 1, 42, false));
   AssertFalse(IsValidRackExtent(0, 1, 42, false));

   EndTest();
}

/**
 * Parse a JSON document from a literal, asserting that it is well formed
 */
static json_t *ParseJson(const char *text)
{
   json_error_t error;
   json_t *json = json_loads(text, 0, &error);
   AssertNotNull(json);
   return json;
}

/**
 * Apply a patch document to given placement and return the result code
 */
static uint32_t ApplyPatch(TestPlacement *p, const char *text, bool allowChassisContainer = true)
{
   json_t *group = ParseJson(text);
   // The placed object is only dereferenced after a container id resolves, which never happens
   // with the FindObjectById stub above, so nullptr is safe here
   uint32_t rcc = ModifyPhysicalPlacementFromJson(group, nullptr, p->ref, allowChassisContainer);
   json_decref(group);
   return rcc;
}

/**
 * Keys present are applied, keys omitted are left untouched
 */
static void TestModifyMergeSemantics()
{
   StartTest(_T("Physical placement merge-patch semantics"));

   TestPlacement p;
   p.position = 12;
   p.height = 3;
   p.orientation = FRONT;

   // Omitted keys must survive
   AssertEquals(ApplyPatch(&p, "{\"position\":20}"), static_cast<uint32_t>(RCC_SUCCESS));
   AssertEquals(static_cast<int32_t>(p.position), 20);
   AssertEquals(static_cast<int32_t>(p.height), 3);
   AssertTrue(p.orientation == FRONT);

   // Empty document is a no-op
   AssertEquals(ApplyPatch(&p, "{}"), static_cast<uint32_t>(RCC_SUCCESS));
   AssertEquals(static_cast<int32_t>(p.position), 20);

   // null containerId unracks
   p.containerId = 326;
   AssertEquals(ApplyPatch(&p, "{\"containerId\":null}"), static_cast<uint32_t>(RCC_SUCCESS));
   AssertEquals(p.containerId, static_cast<uint32_t>(0));

   // Explicit zero unracks too
   p.containerId = 326;
   AssertEquals(ApplyPatch(&p, "{\"containerId\":0}"), static_cast<uint32_t>(RCC_SUCCESS));
   AssertEquals(p.containerId, static_cast<uint32_t>(0));

   EndTest();
}

/**
 * Wrong JSON types and out of range values are rejected
 */
static void TestModifyValidation()
{
   StartTest(_T("Physical placement validation"));

   TestPlacement p;

   AssertEquals(ApplyPatch(&p, "[]"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   AssertEquals(ApplyPatch(&p, "{\"position\":\"36\"}"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   AssertEquals(ApplyPatch(&p, "{\"height\":true}"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   AssertEquals(ApplyPatch(&p, "{\"containerId\":\"326\"}"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   AssertEquals(ApplyPatch(&p, "{\"orientation\":\"front\"}"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));

   // Orientation range is exactly {0, 1, 2}
   AssertEquals(ApplyPatch(&p, "{\"orientation\":3}"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   AssertEquals(ApplyPatch(&p, "{\"orientation\":-1}"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   AssertEquals(ApplyPatch(&p, "{\"orientation\":2}"), static_cast<uint32_t>(RCC_SUCCESS));
   AssertTrue(p.orientation == REAR);

   // Height must be at least one rack unit
   AssertEquals(ApplyPatch(&p, "{\"height\":0}"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   AssertEquals(ApplyPatch(&p, "{\"height\":-2}"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));

   // null clears containerId and the images, but is not a value for the geometry scalars.
   // Height and orientation reject it through their range and type checks; position needs an
   // explicit one, since 0 means "never placed" rather than a rack unit.
   p.position = 12;
   AssertEquals(ApplyPatch(&p, "{\"position\":null}"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   AssertEquals(static_cast<int32_t>(p.position), 12);
   AssertEquals(ApplyPatch(&p, "{\"height\":null}"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   AssertEquals(ApplyPatch(&p, "{\"orientation\":null}"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));

   EndTest();
}

/**
 * Geometry is only validated when the document touches it, so an object whose stored
 * position or height would fail the check stays patchable in other respects
 */
static void TestModifyGeometryScope()
{
   StartTest(_T("Physical placement geometry validation scope"));

   TestPlacement p;
   p.containerId = 326;
   p.position = 0;
   p.height = 0;

   // Neither containerId, position nor height is present: the stored geometry is not looked at,
   // and the unresolvable container id is not looked up either
   AssertEquals(ApplyPatch(&p, "{\"imageFront\":\"fd21a79f-e8d4-4c28-bf07-81b5fce4a26a\"}"), static_cast<uint32_t>(RCC_SUCCESS));
   char buffer[64];
   AssertEquals(p.imageFront.toStringA(buffer), "fd21a79f-e8d4-4c28-bf07-81b5fce4a26a");
   AssertEquals(static_cast<int32_t>(p.position), 0);
   AssertEquals(static_cast<int32_t>(p.height), 0);

   AssertEquals(ApplyPatch(&p, "{\"orientation\":1}"), static_cast<uint32_t>(RCC_SUCCESS));
   AssertTrue(p.orientation == FRONT);

   AssertEquals(ApplyPatch(&p, "{}"), static_cast<uint32_t>(RCC_SUCCESS));

   // Touching geometry resolves the container again, and 326 does not exist here; the stored
   // geometry itself is only judged against a rack
   AssertEquals(ApplyPatch(&p, "{\"position\":4}"), static_cast<uint32_t>(RCC_INVALID_OBJECT_ID));
   AssertEquals(static_cast<int32_t>(p.position), 0);

   EndTest();
}

/**
 * A stored height of 0 - what the console writes for anything placed in a chassis - must not
 * block unplacing, even though an unplace document carries containerId and so brings the
 * geometry checks into scope
 */
static void TestModifyStoredZeroHeight()
{
   StartTest(_T("Physical placement with stored zero height"));

   TestPlacement p;
   p.containerId = 500;
   p.height = 0;

   AssertEquals(ApplyPatch(&p, "{\"containerId\":null}"), static_cast<uint32_t>(RCC_SUCCESS));
   AssertEquals(p.containerId, static_cast<uint32_t>(0));
   AssertEquals(static_cast<int32_t>(p.height), 0);

   // A height sent in the document is still validated, whatever the container is
   AssertEquals(ApplyPatch(&p, "{\"containerId\":null,\"height\":0}"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   AssertEquals(ApplyPatch(&p, "{\"height\":1}"), static_cast<uint32_t>(RCC_SUCCESS));
   AssertEquals(static_cast<int32_t>(p.height), 1);

   EndTest();
}

/**
 * A container id that does not resolve is rejected, and nothing is written
 */
static void TestModifyUnknownContainer()
{
   StartTest(_T("Physical placement with unresolvable container"));

   TestPlacement p;
   p.position = 12;
   p.height = 3;

   AssertEquals(ApplyPatch(&p, "{\"containerId\":9999,\"position\":36}"), static_cast<uint32_t>(RCC_INVALID_OBJECT_ID));

   // Rejected patch must leave the placement exactly as it was
   AssertEquals(p.containerId, static_cast<uint32_t>(0));
   AssertEquals(static_cast<int32_t>(p.position), 12);
   AssertEquals(static_cast<int32_t>(p.height), 3);

   EndTest();
}

/**
 * Image GUIDs: valid strings applied, null clears, malformed rejected
 */
static void TestModifyImages()
{
   StartTest(_T("Physical placement image GUIDs"));

   TestPlacement p;

   AssertEquals(ApplyPatch(&p, "{\"imageFront\":\"fd21a79f-e8d4-4c28-bf07-81b5fce4a26a\"}"), static_cast<uint32_t>(RCC_SUCCESS));
   char buffer[64];
   AssertEquals(p.imageFront.toStringA(buffer), "fd21a79f-e8d4-4c28-bf07-81b5fce4a26a");

   AssertEquals(ApplyPatch(&p, "{\"imageFront\":null}"), static_cast<uint32_t>(RCC_SUCCESS));
   AssertTrue(p.imageFront.isNull());

   AssertEquals(ApplyPatch(&p, "{\"imageRear\":\"not-a-guid\"}"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   AssertTrue(p.imageRear.isNull());

   AssertEquals(ApplyPatch(&p, "{\"imageRear\":42}"), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));

   EndTest();
}

/**
 * Chassis placement XML must round-trip through the same parser Node::getChassisPlacement
 * uses, and must keep the historical "oritentaiton" tag spelling that every client reads.
 */
static void TestChassisPlacementToXml()
{
   StartTest(_T("Chassis placement XML serialization"));

   json_t *placement = ParseJson(
      "{\"image\":\"00000000-0000-0000-0000-000000000001\","
      "\"height\":1,\"heightUnits\":0,\"width\":10,\"widthUnits\":0,"
      "\"positionHeight\":1,\"positionHeightUnits\":0,"
      "\"positionWidth\":2,\"positionWidthUnits\":0,\"orientation\":1}");

   char *xml = ChassisPlacementToXml(placement);
   json_decref(placement);
   AssertNotNull(xml);

   // The misspelling is the contract, and the correct spelling must not appear
   AssertNotNull(strstr(xml, "<oritentaiton>1</oritentaiton>"));
   AssertNull(strstr(xml, "<orientation>"));

   Config config;
   AssertTrue(config.loadXmlConfigFromMemory(xml, strlen(xml), nullptr, "placement", false));
   AssertEquals(config.getValueAsInt(L"/height", -1), 1);
   AssertEquals(config.getValueAsInt(L"/width", -1), 10);
   AssertEquals(config.getValueAsInt(L"/positionWidth", -1), 2);
   AssertEquals(config.getValueAsInt(L"/positionHeight", -1), 1);
   AssertEquals(config.getValueAsInt(L"/oritentaiton", -1), 1);
   char buffer[64];
   AssertEquals(config.getValueAsUUID(L"/image").toStringA(buffer), "00000000-0000-0000-0000-000000000001");

   MemFree(xml);

   // Missing members serialize as zero rather than being omitted, so the document always
   // parses into a complete geometry
   json_t *partial = ParseJson("{\"width\":4}");
   xml = ChassisPlacementToXml(partial);
   json_decref(partial);
   AssertNotNull(xml);
   Config defaults;
   AssertTrue(defaults.loadXmlConfigFromMemory(xml, strlen(xml), nullptr, "placement", false));
   AssertEquals(defaults.getValueAsInt(L"/width", -1), 4);
   AssertEquals(defaults.getValueAsInt(L"/height", -1), 0);
   AssertEquals(defaults.getValueAsInt(L"/oritentaiton", -1), 0);
   MemFree(xml);

   EndTest();
}

/**
 * ValidateChassisPlacementJson must reject a non-integer geometry key or a non-string,
 * non-null image before anything is written - ChassisPlacementToXml itself would silently
 * coerce a bad value to zero instead of failing.
 */
static void TestValidateChassisPlacement()
{
   StartTest(_T("Chassis placement JSON validation"));

   json_t *json = ParseJson(
      "{\"image\":\"00000000-0000-0000-0000-000000000001\","
      "\"height\":1,\"heightUnits\":0,\"width\":10,\"widthUnits\":0,"
      "\"positionHeight\":1,\"positionHeightUnits\":0,"
      "\"positionWidth\":2,\"positionWidthUnits\":0,\"orientation\":1}");
   AssertEquals(ValidateChassisPlacementJson(json), static_cast<uint32_t>(RCC_SUCCESS));
   json_decref(json);

   // Empty object: every key is a merge-patch omission, so nothing to reject
   json = ParseJson("{}");
   AssertEquals(ValidateChassisPlacementJson(json), static_cast<uint32_t>(RCC_SUCCESS));
   json_decref(json);

   // A geometry key as a string coerces to 0 in json_object_get_int32 instead of failing -
   // this is exactly the silent corruption the validator exists to catch
   json = ParseJson("{\"height\":\"abc\"}");
   AssertEquals(ValidateChassisPlacementJson(json), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   json_decref(json);

   json = ParseJson("{\"width\":true}");
   AssertEquals(ValidateChassisPlacementJson(json), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   json_decref(json);

   json = ParseJson("{\"positionHeight\":1.5}");
   AssertEquals(ValidateChassisPlacementJson(json), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   json_decref(json);

   json = ParseJson("{\"positionWidth\":[1,2]}");
   AssertEquals(ValidateChassisPlacementJson(json), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   json_decref(json);

   json = ParseJson("{\"orientation\":{}}");
   AssertEquals(ValidateChassisPlacementJson(json), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   json_decref(json);

   json = ParseJson("{\"heightUnits\":null}");
   AssertEquals(ValidateChassisPlacementJson(json), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   json_decref(json);

   // image accepts a string or null, nothing else
   json = ParseJson("{\"image\":\"00000000-0000-0000-0000-000000000001\"}");
   AssertEquals(ValidateChassisPlacementJson(json), static_cast<uint32_t>(RCC_SUCCESS));
   json_decref(json);

   json = ParseJson("{\"image\":null}");
   AssertEquals(ValidateChassisPlacementJson(json), static_cast<uint32_t>(RCC_SUCCESS));
   json_decref(json);

   json = ParseJson("{\"image\":42}");
   AssertEquals(ValidateChassisPlacementJson(json), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   json_decref(json);

   // A string that is not a UUID would be coerced to the null UUID by json_object_get_uuid,
   // silently wiping the stored image, so it has to be rejected here
   json = ParseJson("{\"image\":\"not-a-guid\"}");
   AssertEquals(ValidateChassisPlacementJson(json), static_cast<uint32_t>(RCC_INVALID_ARGUMENT));
   json_decref(json);

   // Unknown keys are ignored, not rejected - matches the group's merge-patch semantics
   json = ParseJson("{\"bogus\":\"whatever\",\"height\":5}");
   AssertEquals(ValidateChassisPlacementJson(json), static_cast<uint32_t>(RCC_SUCCESS));
   json_decref(json);

   EndTest();
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   TestPlacementToJson();
   TestRackExtent();
   TestModifyMergeSemantics();
   TestModifyValidation();
   TestModifyGeometryScope();
   TestModifyStoredZeroHeight();
   TestModifyUnknownContainer();
   TestModifyImages();
   TestChassisPlacementToXml();
   TestValidateChassisPlacement();

   return 0;
}
