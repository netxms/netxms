/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.netxms.utilities.TestHelper.assertRcc;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.netxms.client.constants.RCC;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyChain;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.client.users.UserGroup;
import org.netxms.utilities.TestHelper;
import org.netxms.utilities.TestHelperForEpp;

/**
 * Access control on event processing policy chains for a user without the global "Edit event processing policy"
 * system right: such a user sees and edits only chains granted through the chain ACL, directly or via a group.
 */
public class EppChainAccessTest extends AbstractSessionTest
{
   private static final String USER_NAME = "EppChainAccessTest-user";
   private static final String USER_PASSWORD = "EppChainAccessTest-1";
   private static final String GROUP_NAME = "EppChainAccessTest-group";
   private static final String CHAIN_HIDDEN = "EppChainAccessTest-Hidden";
   private static final String CHAIN_READABLE = "EppChainAccessTest-Readable";
   private static final String CHAIN_EDITABLE = "EppChainAccessTest-Editable";

   private NXCSession admin;
   private NXCSession user;
   private int userId;
   private int groupId;
   private EventProcessingPolicyChain hidden;
   private EventProcessingPolicyChain readable;
   private EventProcessingPolicyChain editable;

   @BeforeEach
   void setup() throws Exception
   {
      admin = connectAndLogin();
      removeTestObjects();

      // User and group are reused between runs: a deleted user keeps its name until the server purges it
      User u = TestHelper.findOrCreateUser(admin, USER_NAME, USER_PASSWORD);
      userId = u.getId();
      admin.setUserPassword(userId, USER_PASSWORD, null);
      u.setSystemRights(0);
      u.setFlags(u.getFlags() & ~AbstractUserObject.CHANGE_PASSWORD);
      admin.modifyUserDBObject(u, AbstractUserObject.MODIFY_ACCESS_RIGHTS | AbstractUserObject.MODIFY_FLAGS);

      UserGroup group = (UserGroup)admin.findUserDBObjectByName(GROUP_NAME, UserGroup.class);
      if (group == null)
      {
         groupId = admin.createUserGroup(GROUP_NAME);
         admin.syncUserDatabase();
         group = (UserGroup)admin.findUserDBObjectById(groupId, null);
         assertNotNull(group);
      }
      groupId = group.getId();
      group.setMembers(new int[0]);
      admin.modifyUserDBObject(group, AbstractUserObject.MODIFY_MEMBERS);

      hidden = createChain(CHAIN_HIDDEN, null);
      readable = createChain(CHAIN_READABLE, Collections.singletonList(new AccessListElement(userId, EventProcessingPolicyChain.ACCESS_READ)));
      editable = createChain(CHAIN_EDITABLE, Collections.singletonList(new AccessListElement(userId, EventProcessingPolicyChain.ACCESS_READ | EventProcessingPolicyChain.ACCESS_EDIT)));

      user = new NXCSession(TestConstants.SERVER_ADDRESS, TestConstants.SERVER_PORT_CLIENT, true);
      user.connect(new int[] { ProtocolVersion.INDEX_FULL });
      user.login(USER_NAME, USER_PASSWORD);
      assertEquals(0, user.getUserSystemRights() & UserAccessRights.SYSTEM_ACCESS_EPP);
   }

   @AfterEach
   void teardown() throws Exception
   {
      if (user != null)
         user.disconnect();
      if (admin != null)
         removeTestObjects();
   }

   /**
    * Remove chains left by this or a previous run
    */
   private void removeTestObjects() throws Exception
   {
      TestHelperForEpp.deleteChains(admin, "EppChainAccessTest-");
   }

   /**
    * Create chain with one rule and given ACL
    */
   private EventProcessingPolicyChain createChain(String name, List<AccessListElement> acl) throws Exception
   {
      EventProcessingPolicyChain chain = admin.createEppChain(name, "access test chain", acl);
      EventProcessingPolicyRule rule = new EventProcessingPolicyRule();
      rule.setChainId(chain.getId());
      rule.setComments("rule in " + name);
      chain.addRule(rule);
      assertTrue(admin.saveEventProcessingPolicy(chain).isSuccess());
      return chain;
   }

   /**
    * Registry shows only readable chains with effective rights; ACLs are sent only to holders of the global right.
    */
   @Test
   public void testRegistryFiltering() throws Exception
   {
      EventProcessingPolicy policy = user.getEventProcessingPolicyChains();
      assertNull(policy.getMainChain());
      assertNull(policy.findChain(hidden.getId()));

      EventProcessingPolicyChain r = policy.findChain(readable.getId());
      assertNotNull(r);
      assertEquals(EventProcessingPolicyChain.ACCESS_READ, r.getEffectiveRights());
      assertFalse(r.isEditable());
      assertTrue(r.getAccessList().isEmpty());

      EventProcessingPolicyChain e = policy.findChain(editable.getId());
      assertNotNull(e);
      assertEquals(EventProcessingPolicyChain.ACCESS_READ | EventProcessingPolicyChain.ACCESS_EDIT, e.getEffectiveRights());
      assertTrue(e.isEditable());

      // Global right: every chain with full rights and ACL contents
      policy = admin.getEventProcessingPolicyChains();
      assertNotNull(policy.getMainChain());
      assertTrue(policy.getMainChain().isEditable());
      EventProcessingPolicyChain h = policy.findChain(hidden.getId());
      assertNotNull(h);
      assertTrue(h.isEditable());
      assertTrue(h.getAccessList().isEmpty());
      r = policy.findChain(readable.getId());
      assertEquals(1, r.getAccessList().size());
      assertEquals(userId, r.getAccessList().get(0).getUserId());
      assertEquals(EventProcessingPolicyChain.ACCESS_READ, r.getAccessList().get(0).getAccessRights());
   }

   /**
    * Single chain load: readable chains load with effective rights, others and the main chain are denied.
    */
   @Test
   public void testChainLoad() throws Exception
   {
      EventProcessingPolicyChain r = user.getEventProcessingPolicyChain(readable.getId());
      assertEquals(1, r.getRules().size());
      assertEquals(EventProcessingPolicyChain.ACCESS_READ, r.getEffectiveRights());
      assertTrue(r.getAccessList().isEmpty());

      EventProcessingPolicyChain e = user.getEventProcessingPolicyChain(editable.getId());
      assertTrue(e.isEditable());

      assertRcc(RCC.ACCESS_DENIED, () -> user.getEventProcessingPolicyChain(hidden.getId()));
      assertRcc(RCC.ACCESS_DENIED, () -> user.getEventProcessingPolicyChain(0));
      assertRcc(RCC.INVALID_ARGUMENT, () -> admin.getEventProcessingPolicyChain(0x7FFFFFF0));
   }

   /**
    * Save requires Edit on the chain; the main chain requires the global right.
    */
   @Test
   public void testSaveScope() throws Exception
   {
      EventProcessingPolicyChain r = user.getEventProcessingPolicyChain(readable.getId());
      r.getRules().get(0).setComments("changed by limited user");
      assertRcc(RCC.ACCESS_DENIED, () -> user.saveEventProcessingPolicy(r));
      assertEquals("rule in " + CHAIN_READABLE, admin.getEventProcessingPolicyChain(readable.getId()).getRules().get(0).getComments());

      EventProcessingPolicyChain e = user.getEventProcessingPolicyChain(editable.getId());
      e.getRules().get(0).setComments("changed by limited user");
      assertTrue(user.saveEventProcessingPolicy(e).isSuccess());
      assertEquals("changed by limited user", admin.getEventProcessingPolicyChain(editable.getId()).getRules().get(0).getComments());

      // Main chain contents obtained elsewhere cannot be saved without the global right
      EventProcessingPolicyChain main = admin.getEventProcessingPolicyChain(0);
      assertRcc(RCC.ACCESS_DENIED, () -> user.saveEventProcessingPolicy(main));
   }

   /**
    * Without the global right a rule may call only chains the user can read.
    */
   @Test
   public void testChainCallConstraint() throws Exception
   {
      EventProcessingPolicyChain e = user.getEventProcessingPolicyChain(editable.getId());
      e.getRules().get(0).setChainCalls(Arrays.asList(readable.getId()));
      assertTrue(user.saveEventProcessingPolicy(e).isSuccess());
      assertEquals(Arrays.asList(readable.getId()), admin.getEventProcessingPolicyChain(editable.getId()).getRules().get(0).getChainCalls());

      EventProcessingPolicyChain e2 = user.getEventProcessingPolicyChain(editable.getId());
      e2.getRules().get(0).setChainCalls(Arrays.asList(readable.getId(), hidden.getId()));
      assertRcc(RCC.ACCESS_DENIED, () -> user.saveEventProcessingPolicy(e2));
      assertEquals(Arrays.asList(readable.getId()), admin.getEventProcessingPolicyChain(editable.getId()).getRules().get(0).getChainCalls());

      // Global right: calls to any chain
      EventProcessingPolicyChain e3 = admin.getEventProcessingPolicyChain(editable.getId());
      e3.getRules().get(0).setChainCalls(Arrays.asList(hidden.getId()));
      assertTrue(admin.saveEventProcessingPolicy(e3).isSuccess());
   }

   /**
    * Chain create, modify, delete and callers listing require the global right even on an editable chain.
    */
   @Test
   public void testManagementRequiresGlobalRight() throws Exception
   {
      assertRcc(RCC.ACCESS_DENIED, () -> user.createEppChain("EppChainAccessTest-New", "", null));
      assertRcc(RCC.ACCESS_DENIED, () -> user.modifyEppChain(editable.getId(), "renamed", "", null));
      assertRcc(RCC.ACCESS_DENIED, () -> user.deleteEppChain(editable.getId()));
      assertRcc(RCC.ACCESS_DENIED, () -> user.getEppChainCallers(editable.getId()));

      EventProcessingPolicy policy = admin.getEventProcessingPolicyChains();
      assertEquals(CHAIN_EDITABLE, policy.findChain(editable.getId()).getName());
   }

   /**
    * ACL replacement takes effect immediately; absent ACL in modify request keeps the current one.
    */
   @Test
   public void testAclChange() throws Exception
   {
      admin.modifyEppChain(readable.getId(), CHAIN_READABLE, "renamed with ACL kept", null);
      assertNotNull(user.getEventProcessingPolicyChains().findChain(readable.getId()));

      admin.modifyEppChain(readable.getId(), CHAIN_READABLE, "ACL removed", Collections.<AccessListElement>emptyList());
      assertNull(user.getEventProcessingPolicyChains().findChain(readable.getId()));
      assertRcc(RCC.ACCESS_DENIED, () -> user.getEventProcessingPolicyChain(readable.getId()));

      admin.modifyEppChain(readable.getId(), CHAIN_READABLE, "edit granted",
            Collections.singletonList(new AccessListElement(userId, EventProcessingPolicyChain.ACCESS_READ | EventProcessingPolicyChain.ACCESS_EDIT)));
      assertTrue(user.getEventProcessingPolicyChains().findChain(readable.getId()).isEditable());

      // Without any readable chain the policy cannot be opened at all
      admin.modifyEppChain(readable.getId(), CHAIN_READABLE, "", Collections.<AccessListElement>emptyList());
      admin.modifyEppChain(editable.getId(), CHAIN_EDITABLE, "", Collections.<AccessListElement>emptyList());
      assertRcc(RCC.ACCESS_DENIED, () -> user.getEventProcessingPolicyChains());
   }

   /**
    * Group entries grant rights to members; an explicit user entry overrides the group entries.
    */
   @Test
   public void testGroupRights() throws Exception
   {
      admin.syncUserDatabase();
      UserGroup group = (UserGroup)admin.findUserDBObjectById(groupId, null);
      assertNotNull(group);
      group.setMembers(new int[] { userId });
      admin.modifyUserDBObject(group, AbstractUserObject.MODIFY_MEMBERS);

      admin.modifyEppChain(hidden.getId(), CHAIN_HIDDEN, "",
            Collections.singletonList(new AccessListElement(groupId, EventProcessingPolicyChain.ACCESS_READ | EventProcessingPolicyChain.ACCESS_EDIT)));
      EventProcessingPolicyChain h = user.getEventProcessingPolicyChains().findChain(hidden.getId());
      assertNotNull(h);
      assertTrue(h.isEditable());

      // Explicit user entry with Read only wins over the group's Read+Edit
      admin.modifyEppChain(hidden.getId(), CHAIN_HIDDEN, "", Arrays.asList(
            new AccessListElement(groupId, EventProcessingPolicyChain.ACCESS_READ | EventProcessingPolicyChain.ACCESS_EDIT),
            new AccessListElement(userId, EventProcessingPolicyChain.ACCESS_READ)));
      h = user.getEventProcessingPolicyChains().findChain(hidden.getId());
      assertNotNull(h);
      assertFalse(h.isEditable());

      // Removing the user from the group removes group-granted access
      admin.modifyEppChain(hidden.getId(), CHAIN_HIDDEN, "",
            Collections.singletonList(new AccessListElement(groupId, EventProcessingPolicyChain.ACCESS_READ)));
      group.setMembers(new int[0]);
      admin.modifyUserDBObject(group, AbstractUserObject.MODIFY_MEMBERS);
      assertNull(user.getEventProcessingPolicyChains().findChain(hidden.getId()));
   }
}
