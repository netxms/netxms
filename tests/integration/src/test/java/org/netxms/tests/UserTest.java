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
import java.util.concurrent.TimeUnit;
import org.junit.jupiter.api.Test;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.UserGroup;

/**
 * @author Victor
 *
 */
public class UserTest extends AbstractSessionTest
{
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
