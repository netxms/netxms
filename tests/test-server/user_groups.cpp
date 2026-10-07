/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: user_groups.cpp
**
** Tests for effective rights calculation over nested group membership, executed inside the
** test server launcher against the initialized server. Users and groups are created through
** the regular user database API and left in place; the launcher discards its scratch database.
**
** System rights used here are bits that the built-in "Everyone" group does not have by default,
** so a right can only reach a user through the membership under test.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <nms_users.h>
#include <testtools.h>

/**
 * Apply JSON modification to user database object. Takes ownership of the JSON object.
 */
static void ModifyObject(uint32_t id, json_t *json)
{
   json_t *oldData = nullptr;
   json_t *newData = nullptr;
   uint32_t rcc = ModifyUserDatabaseObjectFromJson(id, json, &oldData, &newData);
   json_decref(json);
   json_decref(oldData);
   json_decref(newData);
   AssertEquals(rcc, static_cast<uint32_t>(RCC_SUCCESS));
}

/**
 * Create user or group with given system rights and UI access rules
 */
static uint32_t CreateObject(const TCHAR *name, bool isGroup, uint64_t systemRights, const char *uiAccessRules = nullptr)
{
   uint32_t id = 0;
   AssertEquals(CreateNewUser(name, isGroup, &id), static_cast<uint32_t>(RCC_SUCCESS));

   json_t *json = json_object();
   json_object_set_new(json, "systemRights", json_integer(static_cast<json_int_t>(systemRights)));
   if (uiAccessRules != nullptr)
      json_object_set_new(json, "uiAccessRules", json_string(uiAccessRules));
   ModifyObject(id, json);
   return id;
}

/**
 * Replace member list of a group
 */
static void SetMembers(uint32_t groupId, std::initializer_list<uint32_t> members)
{
   json_t *list = json_array();
   for(uint32_t m : members)
      json_array_append_new(list, json_integer(m));
   json_t *json = json_object();
   json_object_set_new(json, "members", list);
   ModifyObject(groupId, json);
}

/**
 * Set or clear "disabled" flag on a group
 */
static void SetDisabled(uint32_t groupId, bool disabled)
{
   json_t *flags = json_object();
   json_object_set_new(flags, "disabled", json_boolean(disabled));
   json_t *json = json_object();
   json_object_set_new(json, "flags", flags);
   ModifyObject(groupId, json);
}

/**
 * Rights a user gets without any explicit membership (own rights plus "Everyone")
 */
static uint64_t s_baselineRights = 0;

/**
 * Assert that effective rights of given user are exactly baseline plus expected bits
 */
static void AssertRights(uint32_t userId, uint64_t expected)
{
   uint64_t rights = GetEffectiveSystemRights(userId);
   AssertEquals(rights & ~s_baselineRights, expected);
}

/**
 * Nested membership: rights of the group containing the user and of every group above it apply,
 * rights of groups the user does not belong to do not. The sibling branch gets the higher group ID
 * so that a top-down walk visits it first and fails there before finding the user.
 */
static void TestNestedMembership()
{
   StartTest(_T("Nested group membership"));

   uint32_t user = CreateObject(_T("ug_nested_user"), false, SYSTEM_ACCESS_MANAGE_SCRIPTS);
   s_baselineRights = GetEffectiveSystemRights(user) & ~SYSTEM_ACCESS_MANAGE_SCRIPTS;
   AssertTrue((s_baselineRights & (SYSTEM_ACCESS_MANAGE_USERS | SYSTEM_ACCESS_SERVER_CONFIG | SYSTEM_ACCESS_CONFIGURE_TRAPS |
         SYSTEM_ACCESS_MANAGE_SESSIONS | SYSTEM_ACCESS_EDIT_EVENT_DB | SYSTEM_ACCESS_EPP | SYSTEM_ACCESS_MANAGE_ACTIONS |
         SYSTEM_ACCESS_DELETE_ALARMS | SYSTEM_ACCESS_MANAGE_PACKAGES | SYSTEM_ACCESS_VIEW_EVENT_LOG | SYSTEM_ACCESS_MANAGE_TOOLS |
         SYSTEM_ACCESS_MANAGE_SCRIPTS)) == 0);
   AssertRights(user, SYSTEM_ACCESS_MANAGE_SCRIPTS);

   uint32_t inner = CreateObject(_T("ug_nested_inner"), true, SYSTEM_ACCESS_MANAGE_USERS);
   uint32_t leaf = CreateObject(_T("ug_nested_leaf"), true, SYSTEM_ACCESS_EPP);
   uint32_t sibling = CreateObject(_T("ug_nested_sibling"), true, SYSTEM_ACCESS_CONFIGURE_TRAPS);
   uint32_t top = CreateObject(_T("ug_nested_top"), true, SYSTEM_ACCESS_MANAGE_ACTIONS);
   AssertTrue(sibling > inner);

   SetMembers(inner, { user });
   SetMembers(sibling, { leaf });
   SetMembers(top, { inner, sibling });

   AssertRights(user, SYSTEM_ACCESS_MANAGE_SCRIPTS | SYSTEM_ACCESS_MANAGE_USERS | SYSTEM_ACCESS_MANAGE_ACTIONS);
   AssertTrue(CheckUserMembership(user, inner));
   AssertTrue(CheckUserMembership(user, top));
   AssertFalse(CheckUserMembership(user, sibling));
   AssertFalse(CheckUserMembership(user, leaf));

   // Removing the user from the inner group removes the whole chain
   SetMembers(inner, { });
   AssertRights(user, SYSTEM_ACCESS_MANAGE_SCRIPTS);

   EndTest();
}

/**
 * Disabled group neither grants its own rights nor passes membership up to groups containing it
 */
static void TestDisabledGroup()
{
   StartTest(_T("Disabled group in membership chain"));

   uint32_t user = CreateObject(_T("ug_disabled_user"), false, 0);
   uint32_t inner = CreateObject(_T("ug_disabled_inner"), true, SYSTEM_ACCESS_MANAGE_USERS);
   uint32_t outer = CreateObject(_T("ug_disabled_outer"), true, SYSTEM_ACCESS_MANAGE_ACTIONS);
   SetMembers(inner, { user });
   SetMembers(outer, { inner });
   AssertRights(user, SYSTEM_ACCESS_MANAGE_USERS | SYSTEM_ACCESS_MANAGE_ACTIONS);

   SetDisabled(inner, true);
   AssertRights(user, 0);

   SetDisabled(inner, false);
   SetDisabled(outer, true);
   AssertRights(user, SYSTEM_ACCESS_MANAGE_USERS);

   SetDisabled(outer, false);
   AssertRights(user, SYSTEM_ACCESS_MANAGE_USERS | SYSTEM_ACCESS_MANAGE_ACTIONS);

   EndTest();
}

/**
 * Deleted group drops out of the chain immediately, before it is purged from the database
 */
static void TestDeletedGroup()
{
   StartTest(_T("Deleted group in membership chain"));

   uint32_t user = CreateObject(_T("ug_deleted_user"), false, 0);
   uint32_t inner = CreateObject(_T("ug_deleted_inner"), true, SYSTEM_ACCESS_MANAGE_USERS);
   uint32_t outer = CreateObject(_T("ug_deleted_outer"), true, SYSTEM_ACCESS_MANAGE_ACTIONS);
   SetMembers(inner, { user });
   SetMembers(outer, { inner });
   AssertRights(user, SYSTEM_ACCESS_MANAGE_USERS | SYSTEM_ACCESS_MANAGE_ACTIONS);

   AssertEquals(DeleteUserDatabaseObject(inner), static_cast<uint32_t>(RCC_SUCCESS));
   AssertRights(user, 0);

   EndTest();
}

/**
 * Groups nesting each other must not hang the calculation and all of them apply
 */
static void TestMembershipCycle()
{
   StartTest(_T("Cyclic group nesting"));

   uint32_t user = CreateObject(_T("ug_cycle_user"), false, 0);
   uint32_t a = CreateObject(_T("ug_cycle_a"), true, SYSTEM_ACCESS_MANAGE_USERS);
   uint32_t b = CreateObject(_T("ug_cycle_b"), true, SYSTEM_ACCESS_MANAGE_ACTIONS);
   uint32_t c = CreateObject(_T("ug_cycle_c"), true, SYSTEM_ACCESS_MANAGE_PACKAGES);
   uint32_t outside = CreateObject(_T("ug_cycle_outside"), true, SYSTEM_ACCESS_CONFIGURE_TRAPS);
   SetMembers(a, { user, b });
   SetMembers(b, { a });
   SetMembers(c, { b });
   SetMembers(outside, { c, outside });   // self-nesting group not containing the user

   AssertRights(user, SYSTEM_ACCESS_MANAGE_USERS | SYSTEM_ACCESS_MANAGE_ACTIONS | SYSTEM_ACCESS_MANAGE_PACKAGES | SYSTEM_ACCESS_CONFIGURE_TRAPS);
   AssertTrue(CheckUserMembership(user, b));
   AssertTrue(CheckUserMembership(user, outside));

   EndTest();
}

/**
 * "Everyone" contains every user, so a group nesting "Everyone" applies to every user too
 */
static void TestEveryoneNesting()
{
   StartTest(_T("Group nesting \"Everyone\""));

   uint32_t user = CreateObject(_T("ug_everyone_user"), false, 0);
   AssertRights(user, 0);

   uint32_t group = CreateObject(_T("ug_everyone_group"), true, SYSTEM_ACCESS_MANAGE_TOOLS);
   SetMembers(group, { GROUP_EVERYONE });
   AssertRights(user, SYSTEM_ACCESS_MANAGE_TOOLS);
   AssertTrue(CheckUserMembership(user, group));

   uint32_t other = CreateObject(_T("ug_everyone_other"), false, 0);
   AssertRights(other, SYSTEM_ACCESS_MANAGE_TOOLS);

   SetMembers(group, { });
   AssertRights(user, 0);

   EndTest();
}

/**
 * UI access rules: user's own rules first, then rules of every group the user belongs to, in group ID order
 */
static void TestUIAccessRules()
{
   StartTest(_T("Effective UI access rules"));

   uint32_t user = CreateObject(_T("ug_ui_user"), false, 0, "user-rule");
   String baseline = GetEffectiveUIAccessRules(user);   // own rule plus "Everyone"
   AssertTrue(baseline.startsWith(_T("user-rule")));

   uint32_t inner = CreateObject(_T("ug_ui_inner"), true, 0, "inner-rule");
   uint32_t outer = CreateObject(_T("ug_ui_outer"), true, 0, "outer-rule");
   uint32_t unrelated = CreateObject(_T("ug_ui_unrelated"), true, 0, "unrelated-rule");
   SetMembers(inner, { user, unrelated });   // "unrelated" is a sibling of the user inside "inner" and does not contain the user
   SetMembers(outer, { inner });

   StringBuffer expected(baseline);
   expected.append(_T(";inner-rule;outer-rule"));
   String rules = GetEffectiveUIAccessRules(user);
   AssertEquals(rules.cstr(), expected.cstr());

   EndTest();
}

/**
 * Deep chain of nested groups: each group nests the previous one. A walk that re-enters shared
 * subtrees once per ancestor is cubic in chain length here, a single bottom-up pass is linear.
 */
static void TestDeepChain()
{
   StartTest(_T("Deep nested group chain"));

   const int chainLength = 2000;

   uint32_t user = CreateObject(_T("ug_chain_user"), false, 0);
   uint32_t previous = 0;
   uint32_t last = 0;
   for(int i = 0; i < chainLength; i++)
   {
      TCHAR name[64];
      _sntprintf(name, 64, _T("ug_chain_%04d"), i);
      uint32_t group = CreateObject(name, true, (i == chainLength - 1) ? SYSTEM_ACCESS_DELETE_ALARMS : 0);
      if (i == 0)
         SetMembers(group, { user });
      else
         SetMembers(group, { previous });
      previous = group;
      last = group;
   }

   int64_t startTime = GetCurrentTimeMs();
   AssertRights(user, SYSTEM_ACCESS_DELETE_ALARMS);
   int64_t elapsed = GetCurrentTimeMs() - startTime;
   AssertTrue(elapsed < 1000);
   AssertTrue(CheckUserMembership(user, last));

   EndTest();
}

/**
 * Test entry point
 */
void TestUserGroups()
{
   TestNestedMembership();
   TestDisabledGroup();
   TestDeletedGroup();
   TestMembershipCycle();
   TestEveryoneNesting();
   TestUIAccessRules();
   TestDeepChain();
}
