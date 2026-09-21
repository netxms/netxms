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
** File: object_hierarchy.cpp
**
** Tests for object-specific binding rules (ValidateObjectBinding / NetObj::validateParent)
** of the data center spatial model classes.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <testtools.h>

/**
 * Create object of given class with given name and insert it into object indexes
 */
template<typename T> static shared_ptr<T> CreateObject(const TCHAR *name)
{
   shared_ptr<T> object = make_shared<T>();
   object->setName(name);
   NetObjInsert(object, true, false);
   return object;
}

/**
 * Rack may have at most one cooling zone parent
 */
static void TestRackCoolingZoneRule()
{
   StartTest(_T("Rack may have at most one cooling zone parent"));

   shared_ptr<CoolingZone> zone1 = CreateObject<CoolingZone>(_T("cz-1"));
   shared_ptr<CoolingZone> zone2 = CreateObject<CoolingZone>(_T("cz-2"));
   shared_ptr<Rack> rack = make_shared<Rack>(_T("rack-1"), 42);
   NetObjInsert(rack, true, false);

   AssertEquals(ValidateObjectBinding(*rack, *zone1), RCC_SUCCESS);
   NetObj::linkObjects(zone1, rack);
   AssertEquals(ValidateObjectBinding(*rack, *zone1), RCC_SUCCESS);   // Same zone again is not a violation
   AssertEquals(ValidateObjectBinding(*rack, *zone2), RCC_OBJECT_HIERARCHY_VIOLATION);

   EndTest();
}

/**
 * Power domain may not have facility parent and power domain parent from a different facility
 */
static void TestPowerDomainFacilityRule()
{
   StartTest(_T("Power domain with facility parent and power domain parent from different facility"));

   shared_ptr<Facility> facility1 = CreateObject<Facility>(_T("facility-1"));
   shared_ptr<Facility> facility2 = CreateObject<Facility>(_T("facility-2"));
   shared_ptr<PowerDomain> domain = CreateObject<PowerDomain>(_T("pd-x"));
   shared_ptr<PowerDomain> parent1 = CreateObject<PowerDomain>(_T("pd-parent-1"));
   shared_ptr<PowerDomain> parent2 = CreateObject<PowerDomain>(_T("pd-parent-2"));
   NetObj::linkObjects(facility1, domain);
   NetObj::linkObjects(facility1, parent1);
   NetObj::linkObjects(facility2, parent2);

   AssertEquals(ValidateObjectBinding(*domain, *parent1), RCC_SUCCESS);   // Same facility
   AssertEquals(ValidateObjectBinding(*domain, *parent2), RCC_OBJECT_HIERARCHY_VIOLATION);

   // Facility parent added to domain that already has power domain parent from other facility
   shared_ptr<PowerDomain> domain2 = CreateObject<PowerDomain>(_T("pd-y"));
   NetObj::linkObjects(parent2, domain2);
   AssertEquals(ValidateObjectBinding(*domain2, *facility2), RCC_SUCCESS);
   AssertEquals(ValidateObjectBinding(*domain2, *facility1), RCC_OBJECT_HIERARCHY_VIOLATION);

   EndTest();
}

/**
 * Direct membership in two facilities is not a bind-time error
 */
static void TestPowerDomainTwoFacilitiesAllowed()
{
   StartTest(_T("Power domain directly under two facilities is allowed"));

   shared_ptr<Facility> facility1 = CreateObject<Facility>(_T("facility-3"));
   shared_ptr<Facility> facility2 = CreateObject<Facility>(_T("facility-4"));
   shared_ptr<PowerDomain> domain = CreateObject<PowerDomain>(_T("pd-z"));
   NetObj::linkObjects(facility1, domain);

   AssertEquals(ValidateObjectBinding(*domain, *facility2), RCC_SUCCESS);

   // Power domain parent that is itself in both facilities is not a conflict for a domain in both facilities
   shared_ptr<PowerDomain> parent = CreateObject<PowerDomain>(_T("pd-parent-3"));
   NetObj::linkObjects(facility1, parent);
   NetObj::linkObjects(facility2, parent);
   NetObj::linkObjects(facility2, domain);
   AssertEquals(ValidateObjectBinding(*domain, *parent), RCC_SUCCESS);

   EndTest();
}

/**
 * Rule is evaluated through chains of power domains and for descendants of the bound object
 */
static void TestPowerDomainChainAndDescendants()
{
   StartTest(_T("Power domain rule through chains and for descendants"));

   shared_ptr<Facility> facility1 = CreateObject<Facility>(_T("facility-5"));
   shared_ptr<Facility> facility2 = CreateObject<Facility>(_T("facility-6"));
   shared_ptr<PowerDomain> top = CreateObject<PowerDomain>(_T("pd-top"));
   shared_ptr<PowerDomain> middle = CreateObject<PowerDomain>(_T("pd-middle"));
   shared_ptr<PowerDomain> leaf = CreateObject<PowerDomain>(_T("pd-leaf"));
   NetObj::linkObjects(top, middle);
   NetObj::linkObjects(middle, leaf);
   NetObj::linkObjects(facility1, leaf);

   // Nothing above leaf is in a facility yet
   AssertEquals(ValidateObjectBinding(*top, *facility1), RCC_SUCCESS);
   AssertEquals(ValidateObjectBinding(*middle, *facility1), RCC_SUCCESS);

   // Binding an ancestor to another facility would put leaf under facility-5 with a chain from facility-6
   AssertEquals(ValidateObjectBinding(*top, *facility2), RCC_OBJECT_HIERARCHY_VIOLATION);
   AssertEquals(ValidateObjectBinding(*middle, *facility2), RCC_OBJECT_HIERARCHY_VIOLATION);

   // Same through a new power domain parent that belongs to another facility
   shared_ptr<PowerDomain> other = CreateObject<PowerDomain>(_T("pd-other"));
   NetObj::linkObjects(facility2, other);
   AssertEquals(ValidateObjectBinding(*top, *other), RCC_OBJECT_HIERARCHY_VIOLATION);

   // Ancestor without facility lineage is fine as a new parent
   shared_ptr<PowerDomain> neutral = CreateObject<PowerDomain>(_T("pd-neutral"));
   AssertEquals(ValidateObjectBinding(*top, *neutral), RCC_SUCCESS);

   // Once top is in facility-5 the chain is consistent
   NetObj::linkObjects(facility1, top);
   AssertEquals(ValidateObjectBinding(*leaf, *facility1), RCC_SUCCESS);
   AssertEquals(ValidateObjectBinding(*leaf, *facility2), RCC_SUCCESS);   // Direct second facility is only a warning

   EndTest();
}

/**
 * Room containment rules: rack may have at most one room parent, rooms do not nest
 */
static void TestRoomContainmentRules()
{
   StartTest(_T("Room containment rules"));

   shared_ptr<Room> room1 = CreateObject<Room>(_T("room-1"));
   shared_ptr<Room> room2 = CreateObject<Room>(_T("room-2"));
   shared_ptr<Facility> facility = CreateObject<Facility>(_T("facility-room"));
   shared_ptr<Container> container = make_shared<Container>(_T("container-room"));
   NetObjInsert(container, true, false);
   shared_ptr<CoolingZone> zone = CreateObject<CoolingZone>(_T("cz-room"));
   shared_ptr<PowerDomain> domain = CreateObject<PowerDomain>(_T("pd-room"));
   shared_ptr<Rack> rack = make_shared<Rack>(_T("rack-room"), 42);
   NetObjInsert(rack, true, false);

   AssertEquals(ValidateObjectBinding(*room1, *facility), RCC_SUCCESS);
   AssertEquals(ValidateObjectBinding(*room1, *container), RCC_SUCCESS);
   AssertEquals(ValidateObjectBinding(*room1, *room2), RCC_INCOMPATIBLE_OPERATION);
   AssertEquals(ValidateObjectBinding(*room1, *zone), RCC_INCOMPATIBLE_OPERATION);
   AssertEquals(ValidateObjectBinding(*room1, *domain), RCC_INCOMPATIBLE_OPERATION);
   AssertEquals(ValidateObjectBinding(*container, *room1), RCC_INCOMPATIBLE_OPERATION);

   AssertEquals(ValidateObjectBinding(*rack, *room1), RCC_SUCCESS);
   NetObj::linkObjects(room1, rack);
   AssertEquals(ValidateObjectBinding(*rack, *room1), RCC_SUCCESS);   // Same room again is not a violation
   AssertEquals(ValidateObjectBinding(*rack, *room2), RCC_OBJECT_HIERARCHY_VIOLATION);
   AssertEquals(ValidateObjectBinding(*rack, *zone), RCC_SUCCESS);    // Spatial and thermal parents are independent

   EndTest();
}

/**
 * Rack footprint defaults and floor plan placement flag handling
 */
static void TestRackRoomPlacement()
{
   StartTest(_T("Rack footprint defaults and room placement flag"));

   shared_ptr<Room> room = CreateObject<Room>(_T("room-3"));
   shared_ptr<Rack> rack = make_shared<Rack>(_T("rack-placement"), 0);
   NetObjInsert(rack, true, false);
   AssertEquals(rack->getHeight(), 42);
   AssertEquals(rack->getWidth(), DEFAULT_RACK_WIDTH);
   AssertEquals(rack->getDepth(), DEFAULT_RACK_DEPTH);
   AssertFalse(rack->isPlacedInRoom());

   shared_ptr<Rack> wideRack = make_shared<Rack>(_T("rack-wide"), 47, 800, 1200);
   AssertEquals(wideRack->getWidth(), 800);
   AssertEquals(wideRack->getDepth(), 1200);

   NetObj::linkObjects(room, rack);
   json_t *json = json_pack("{s:{s:b,s:i,s:i,s:i}}", "roomPlacement", "placed", 1, "x", 1200, "y", 2400, "rotation", 450);
   AssertEquals(rack->modifyFromJSON(json, nullptr), RCC_SUCCESS);
   json_decref(json);
   AssertTrue(rack->isPlacedInRoom());
   AssertEquals(rack->getRoomX(), 1200);
   AssertEquals(rack->getRoomY(), 2400);
   AssertEquals(rack->getRoomRotation(), 90);

   json = json_pack("{s:{s:i}}", "roomPlacement", "rotation", -90);
   AssertEquals(rack->modifyFromJSON(json, nullptr), RCC_SUCCESS);
   json_decref(json);
   AssertEquals(rack->getRoomRotation(), 270);

   json = json_pack("{s:i}", "width", 0);
   AssertEquals(rack->modifyFromJSON(json, nullptr), RCC_INVALID_ARGUMENT);
   json_decref(json);
   AssertEquals(rack->getWidth(), DEFAULT_RACK_WIDTH);

   // Placement is only valid for the room it was made in
   NetObj::unlinkObjects(room.get(), rack.get());
   AssertFalse(rack->isPlacedInRoom());

   EndTest();
}

/**
 * Room outline handling and floor area calculation
 */
static void TestRoomOutline()
{
   StartTest(_T("Room outline and floor area"));

   // Rectangular outline generated from width and depth
   json_t *json = json_pack("{s:s,s:i,s:i}", "roomType", "COMPUTER_ROOM", "width", 12000, "depth", 8000);
   shared_ptr<Room> room = make_shared<Room>(_T("room-rect"), json);
   json_decref(json);
   AssertEquals(static_cast<int32_t>(room->getRoomType()), static_cast<int32_t>(ROOM_COMPUTER_ROOM));
   AssertEquals(room->getOutline().size(), 4);
   AssertTrue(fabs(room->getArea() - 96.0) < 0.000001);

   // L-shaped outline: 10 x 10 m with 4 x 6 m notch, vertices in clockwise order
   json = json_pack("{s:{s:[{s:i,s:i},{s:i,s:i},{s:i,s:i},{s:i,s:i},{s:i,s:i},{s:i,s:i}]}}", "room", "outline",
      "x", 0, "y", 0, "x", 0, "y", 10000, "x", 6000, "y", 10000, "x", 6000, "y", 4000, "x", 10000, "y", 4000, "x", 10000, "y", 0);
   AssertEquals(room->NetObj::modifyFromJSON(json, nullptr), RCC_SUCCESS);
   json_decref(json);
   AssertEquals(room->getOutline().size(), 6);
   AssertTrue(fabs(room->getArea() - 76.0) < 0.000001);

   // Invalid outline is rejected and leaves existing one intact
   json = json_pack("{s:{s:[{s:i,s:i},{s:i,s:i}]}}", "room", "outline", "x", 0, "y", 0, "x", 1000, "y", 1000);
   AssertEquals(room->NetObj::modifyFromJSON(json, nullptr), RCC_INVALID_ARGUMENT);
   json_decref(json);
   AssertEquals(room->getOutline().size(), 6);

   json = json_pack("{s:{s:s}}", "room", "roomType", "KITCHEN");
   AssertEquals(room->NetObj::modifyFromJSON(json, nullptr), RCC_INVALID_ARGUMENT);
   json_decref(json);

   EndTest();
}

/**
 * Room and rack placement attributes and creation methods in NXSL
 */
static void TestRoomNXSL()
{
   StartTest(_T("Room and rack placement in NXSL"));

   shared_ptr<Container> container = make_shared<Container>(_T("container-nxsl-room"));
   NetObjInsert(container, true, false);

   static const wchar_t *script =
      L"room = $object.createRoom(\"nxsl-room\", 0, 12000, 8000, 3000);\n"
      L"assert(room != null);\n"
      L"assert(classof(room) == \"Room\");\n"
      L"assert(room.roomType == 0);\n"
      L"assert(room.roomTypeName == \"COMPUTER_ROOM\");\n"
      L"assert(room.height == 3000);\n"
      L"assert(room.area == 96);\n"
      L"assert(room.gridTileSize == 0);\n"
      L"assert(room.gridLabelsName == \"NONE\");\n"
      L"assert(room.backgroundImage == null);\n"
      L"assert(room.outline.size == 4);\n"
      L"assert(room.outline[2][\"x\"] == 12000);\n"
      L"assert(room.outline[2][\"y\"] == 8000);\n"
      L"assert(room.isAutoBindEnabled == false);\n"
      L"rack = room.createRack(\"nxsl-rack\");\n"
      L"assert(rack.height == 42);\n"
      L"assert(rack.width == 600);\n"
      L"assert(rack.depth == 1000);\n"
      L"assert(rack.isPlacedInRoom == false);\n"
      L"assert(rack.roomX == 0);\n"
      L"assert(rack.roomRotation == 0);\n"
      L"assert(rack.room.id == room.id);\n"
      L"wideRack = $object.createRack(\"nxsl-rack-wide\", 47, 800, 1200);\n"
      L"assert(wideRack.height == 47);\n"
      L"assert(wideRack.width == 800);\n"
      L"assert(wideRack.depth == 1200);\n"
      L"assert(wideRack.room == null);\n";

   NXSL_CompilationDiagnostic diag;
   NXSL_VM *vm = NXSLCompileAndCreateVM(script, new NXSL_ServerEnv(), &diag);
   if (vm == nullptr)
      WriteToTerminalEx(L"\n   Compilation error: %s\n", diag.errorText.cstr());
   AssertNotNull(vm);
   SetupServerScriptVM(vm, container, shared_ptr<DCObjectInfo>());
   bool success = vm->run();
   if (!success)
      WriteToTerminalEx(L"\n   Script error: %s\n", vm->getErrorText());
   AssertTrue(success);
   delete vm;

   EndTest();
}

/**
 * Entry point
 */
void TestObjectHierarchy()
{
   TestRackCoolingZoneRule();
   TestPowerDomainFacilityRule();
   TestPowerDomainTwoFacilitiesAllowed();
   TestPowerDomainChainAndDescendants();
   TestRoomContainmentRules();
   TestRackRoomPlacement();
   TestRoomOutline();
   TestRoomNXSL();
}
