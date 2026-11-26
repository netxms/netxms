/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
import java.util.ArrayDeque;
import java.util.Arrays;
import java.util.Deque;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import org.junit.jupiter.api.Test;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.RCC;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.client.users.UserGroup;

/**
 * @author Victor
 *
 */
public class UserTest extends AbstractSessionTest
{
   @Test
   public void testGroupMembershipUpdatesClientRights() throws Exception
   {
      NXCSession admin = connectAndLogin();
      String suffix = UUID.randomUUID().toString();
      Deque<Integer> principals = new ArrayDeque<>();
      long containerId = 0;
      try
      {
         int userId = admin.createUser("member-" + suffix);
         principals.push(userId);
         int readersId = admin.createUserGroup("readers-" + suffix);
         principals.push(readersId);
         int groupId = admin.createUserGroup("scripters-" + suffix);
         principals.push(groupId);
         admin.setUserPassword(userId, "Rights-Test-1", null);
         admin.syncUserDatabase();
         User user = (User)admin.findUserDBObjectById(userId, null);
         user.setFlags(user.getFlags() & ~AbstractUserObject.CHANGE_PASSWORD);
         admin.modifyUserDBObject(user, AbstractUserObject.MODIFY_FLAGS);
         UserGroup readers = (UserGroup)admin.findUserDBObjectById(readersId, null);
         readers.setMembers(new int[] { userId });
         admin.modifyUserDBObject(readers, AbstractUserObject.MODIFY_MEMBERS);
         UserGroup group = (UserGroup)admin.findUserDBObjectById(groupId, null);

         // Rights come from groups only: a direct entry for the user would take precedence over group entries
         containerId = admin.createObject(new NXCObjectCreationData(AbstractObject.OBJECT_CONTAINER, "membership-" + suffix, AbstractObject.SERVICEROOT));
         NXCObjectModificationData acl = new NXCObjectModificationData(containerId);
         acl.setInheritAccessRights(false);
         acl.setACL(Arrays.asList(new AccessListElement(admin.getUserId(), admin.getEffectiveRights(containerId)),
               new AccessListElement(readersId, UserAccessRights.OBJECT_ACCESS_READ),
               new AccessListElement(groupId, UserAccessRights.OBJECT_ACCESS_EXECUTE_SCRIPT)));
         admin.modifyObject(acl);

         NXCSession session = new NXCSession(TestConstants.SERVER_ADDRESS, TestConstants.SERVER_PORT_CLIENT, true);
         session.connect(new int[] { ProtocolVersion.INDEX_FULL });
         try
         {
            session.login(user.getName(), "Rights-Test-1");
            session.syncObjects();
            AbstractObject container = session.findObjectById(containerId);
            assertNotNull(container);
            assertEquals(Long.valueOf(UserAccessRights.OBJECT_ACCESS_READ), container.getCachedEffectiveRights());
            long systemRights = session.getUserSystemRights();

            // Membership change alone does not change the object, so instance received at login must follow it
            group.setMembers(new int[] { userId });
            admin.modifyUserDBObject(group, AbstractUserObject.MODIFY_MEMBERS);
            assertClientRights(container, UserAccessRights.OBJECT_ACCESS_READ | UserAccessRights.OBJECT_ACCESS_EXECUTE_SCRIPT);

            group.setMembers(new int[0]);
            admin.modifyUserDBObject(group, AbstractUserObject.MODIFY_MEMBERS);
            assertClientRights(container, UserAccessRights.OBJECT_ACCESS_READ);
            assertEquals(systemRights, session.getUserSystemRights());
         }
         finally
         {
            session.disconnect();
         }
      }
      finally
      {
         if (containerId != 0)
            admin.deleteObject(containerId);
         for(int id : principals)
            admin.deleteUserDBObject(id);
      }
   }

   private static void assertClientRights(AbstractObject object, long expected) throws Exception
   {
      long start = System.nanoTime();
      long rights = object.getEffectiveRights();
      while((rights != expected) && (System.nanoTime() - start < TimeUnit.SECONDS.toNanos(10)))
      {
         Thread.sleep(100);
         rights = object.getEffectiveRights();
      }
      assertEquals(expected, rights);
   }

   @Test
   public void testObjectRightsNotifications() throws Exception
   {
      NXCSession admin = connectAndLogin();
      BlockingQueue<Long> generations = new LinkedBlockingQueue<>();
      SessionListener listener = notification -> {
         if (notification.getCode() == SessionNotification.OBJECT_RIGHTS_CHANGED)
            generations.add(notification.getSubCode());
      };
      String suffix = UUID.randomUUID().toString();
      int userId = admin.createUser("member-" + suffix);
      try
      {
         int groupId = admin.createUserGroup("group-" + suffix);
         try
         {
            admin.syncUserDatabase();
            User user = (User)admin.findUserDBObjectById(userId, null);
            UserGroup group = (UserGroup)admin.findUserDBObjectById(groupId, null);
            assertNotNull(user);
            assertNotNull(group);
            admin.addListener(listener);

            group.setMembers(new int[] { userId });
            admin.modifyUserDBObject(group, AbstractUserObject.MODIFY_MEMBERS);
            long added = awaitGeneration(generations);

            admin.modifyUserDBObject(group, AbstractUserObject.MODIFY_MEMBERS);  // unchanged
            group.setSystemRights(UserAccessRights.SYSTEM_ACCESS_VIEW_EVENT_LOG);
            admin.modifyUserDBObject(group, AbstractUserObject.MODIFY_ACCESS_RIGHTS);
            assertNull(generations.poll(500, TimeUnit.MILLISECONDS));

            user.setGroups(new int[0]);
            admin.modifyUserDBObject(user, AbstractUserObject.MODIFY_GROUP_MEMBERSHIP);
            long removed = awaitGeneration(generations);
            assertEquals(added + 1, removed);

            group.setFlags(group.getFlags() | AbstractUserObject.DISABLED);
            admin.modifyUserDBObject(group, AbstractUserObject.MODIFY_FLAGS);
            assertEquals(removed + 1, awaitGeneration(generations));
            assertNull(generations.poll(500, TimeUnit.MILLISECONDS));
         }
         finally
         {
            admin.removeListener(listener);
            admin.deleteUserDBObject(groupId);
         }
      }
      finally
      {
         admin.deleteUserDBObject(userId);
      }
   }

   private static long awaitGeneration(BlockingQueue<Long> generations) throws InterruptedException
   {
      Long generation = generations.poll(10, TimeUnit.SECONDS);
      assertNotNull(generation, "Expected an object rights notification");
      return generation;
   }

   @Test
   public void testGroupStateUpdatesInheritedRights() throws Exception
   {
      NXCSession admin = connectAndLogin();
      String suffix = UUID.randomUUID().toString();
      Deque<Integer> principals = new ArrayDeque<>();
      long parentId = 0;
      try
      {
         int userId = admin.createUser("member-" + suffix);
         principals.push(userId);
         int outerId = admin.createUserGroup("outer-" + suffix);
         principals.push(outerId);
         int innerId = admin.createUserGroup("inner-" + suffix);
         principals.push(innerId);
         admin.syncUserDatabase();
         UserGroup outer = (UserGroup)admin.findUserDBObjectById(outerId, null);
         UserGroup inner = (UserGroup)admin.findUserDBObjectById(innerId, null);
         assertNotNull(outer);
         assertNotNull(inner);
         inner.setMembers(new int[] { userId });
         admin.modifyUserDBObject(inner, AbstractUserObject.MODIFY_MEMBERS);
         outer.setMembers(new int[] { innerId });
         admin.modifyUserDBObject(outer, AbstractUserObject.MODIFY_MEMBERS);

         parentId = admin.createObject(new NXCObjectCreationData(AbstractObject.OBJECT_CONTAINER, "group-state-" + suffix, AbstractObject.SERVICEROOT));
         NXCObjectModificationData acl = new NXCObjectModificationData(parentId);
         acl.setInheritAccessRights(false);
         acl.setACL(Arrays.asList(new AccessListElement(admin.getUserId(), admin.getEffectiveRights(parentId)),
               new AccessListElement(outerId, UserAccessRights.OBJECT_ACCESS_READ)));
         admin.modifyObject(acl);
         long childId = admin.createObject(new NXCObjectCreationData(AbstractObject.OBJECT_CONTAINER, "child", parentId));
         assertInheritedRights(admin, childId, userId, UserAccessRights.OBJECT_ACCESS_READ);

         inner.setFlags(inner.getFlags() | AbstractUserObject.DISABLED);
         admin.modifyUserDBObject(inner, AbstractUserObject.MODIFY_FLAGS);
         assertInheritedRights(admin, childId, userId, 0);
         inner.setFlags(inner.getFlags() & ~AbstractUserObject.DISABLED);
         admin.modifyUserDBObject(inner, AbstractUserObject.MODIFY_FLAGS);
         assertInheritedRights(admin, childId, userId, UserAccessRights.OBJECT_ACCESS_READ);

         admin.deleteUserDBObject(innerId);
         principals.pop();
         assertInheritedRights(admin, childId, userId, 0);
      }
      finally
      {
         if (parentId != 0)
            admin.deleteObject(parentId);
         for(int id : principals)
            admin.deleteUserDBObject(id);
      }
   }

   private static void assertInheritedRights(NXCSession admin, long objectId, int userId, long expected) throws Exception
   {
      long start = System.nanoTime();
      long rights = admin.getEffectiveRights(objectId, userId);
      while((rights != expected) && (System.nanoTime() - start < TimeUnit.SECONDS.toNanos(10)))
      {
         Thread.sleep(100);
         rights = admin.getEffectiveRights(objectId, userId);
      }
      assertEquals(expected, rights);
   }

   @Test
   public void testValidatePassword() throws Exception
   {
      final NXCSession session = connectAndLogin();

      assertTrue(session.validateUserPassword(TestConstants.SERVER_PASSWORD));
   }

   @Test
   public void testUserVariables() throws Exception
   {
      final NXCSession session = connectAndLogin();
      final String name = "IntegrationTest.UserVariable";

      session.deleteUserVariable(name);
      assertNull(session.getUserVariable(name));

      session.setUserVariable(name, "test value");
      assertEquals("test value", session.getUserVariable(name));

      session.setUserVariable(name, "updated value");
      assertEquals("updated value", session.getUserVariable(name));

      Map<String, String> variables = session.getUserVariables();
      assertEquals("updated value", variables.get(name));

      session.deleteUserVariable(name);
      assertNull(session.getUserVariable(name));
      assertFalse(session.getUserVariables().containsKey(name));

      session.deleteUserVariable(name); // deleting non-existent variable is not an error

      boolean invalidName = false;
      try
      {
         session.setUserVariable("invalid name!", "test");
      }
      catch(NXCException e)
      {
         assertEquals(RCC.INVALID_OBJECT_NAME, e.getErrorCode());
         invalidName = true;
      }
      assertTrue(invalidName);
   }
}
