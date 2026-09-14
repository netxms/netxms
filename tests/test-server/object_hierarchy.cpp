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
 * Entry point
 */
void TestObjectHierarchy()
{
   TestRackCoolingZoneRule();
   TestPowerDomainFacilityRule();
   TestPowerDomainTwoFacilitiesAllowed();
   TestPowerDomainChainAndDescendants();
}
