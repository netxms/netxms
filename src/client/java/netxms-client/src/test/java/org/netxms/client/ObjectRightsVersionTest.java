/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.client;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertSame;
import org.junit.jupiter.api.Test;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.GenericObject;

/**
 * Tests for object rights versioning
 */
public class ObjectRightsVersionTest
{
   private static AbstractObject createObject(NXCSession session, long rights, long generation)
   {
      NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_OBJECT);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, 100);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_CLASS, AbstractObject.OBJECT_CONTAINER);
      msg.setFieldInt64(NXCPCodes.VID_EFFECTIVE_RIGHTS, rights);
      msg.setFieldInt64(NXCPCodes.VID_RIGHTS_GENERATION, generation);
      return new GenericObject(msg, session);
   }

   @Test
   public void testGenerationOrdering()
   {
      NXCSession session = new NXCSession("127.0.0.1");
      ObjectRightsVersion current = session.acceptObjectRightsGeneration(5);
      assertEquals(5, current.getGeneration());
      assertSame(current, session.acceptObjectRightsGeneration(5));
      assertNull(session.acceptObjectRightsGeneration(4));
      assertSame(current, session.getObjectRightsVersion());
   }

   @Test
   public void testLazyFetchDiscardsOutdatedReply()
   {
      NXCSession session = new NXCSession("127.0.0.1") {
         private int calls = 0;

         @Override
         public long getEffectiveRights(long objectId)
         {
            // First reply was calculated before membership change reported by server during request
            if (calls++ == 0)
            {
               acceptObjectRightsGeneration(2);
               return UserAccessRights.OBJECT_ACCESS_READ | UserAccessRights.OBJECT_ACCESS_EXECUTE_SCRIPT;
            }
            return UserAccessRights.OBJECT_ACCESS_READ;
         }
      };
      session.acceptObjectRightsGeneration(1);
      NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_OBJECT);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, 100);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_CLASS, AbstractObject.OBJECT_CONTAINER);
      AbstractObject object = new GenericObject(msg, session);

      assertEquals(UserAccessRights.OBJECT_ACCESS_READ, object.getEffectiveRights());
      assertEquals(Long.valueOf(UserAccessRights.OBJECT_ACCESS_READ), object.getCachedEffectiveRights());
   }

   @Test
   public void testLazyFetchUnderSustainedChanges()
   {
      NXCSession session = new NXCSession("127.0.0.1") {
         private int calls = 0;

         @Override
         public long getEffectiveRights(long objectId)
         {
            // Every one of first three replies is overtaken by another membership change
            calls++;
            if (calls <= 3)
            {
               acceptObjectRightsGeneration(calls + 1);
               return UserAccessRights.OBJECT_ACCESS_READ | UserAccessRights.OBJECT_ACCESS_EXECUTE_SCRIPT;
            }
            return UserAccessRights.OBJECT_ACCESS_READ;
         }
      };
      session.acceptObjectRightsGeneration(1);
      NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_OBJECT);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, 100);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_CLASS, AbstractObject.OBJECT_CONTAINER);
      AbstractObject object = new GenericObject(msg, session);

      assertEquals(0, object.getEffectiveRights());
      assertNull(object.getCachedEffectiveRights());
      assertEquals(UserAccessRights.OBJECT_ACCESS_READ, object.getEffectiveRights());
      assertEquals(Long.valueOf(UserAccessRights.OBJECT_ACCESS_READ), object.getCachedEffectiveRights());
   }

   @Test
   public void testPushedRightsInvalidation()
   {
      NXCSession session = new NXCSession("127.0.0.1");
      long granted = UserAccessRights.OBJECT_ACCESS_READ | UserAccessRights.OBJECT_ACCESS_EXECUTE_SCRIPT;
      AbstractObject held = createObject(session, granted, 5);
      assertEquals(Long.valueOf(granted), held.getCachedEffectiveRights());

      // Notification about membership change makes rights held by any instance outdated
      session.acceptObjectRightsGeneration(6);
      assertNull(held.getCachedEffectiveRights());

      // Object serialized before membership change and delivered after notification cannot restore revoked right
      AbstractObject late = createObject(session, granted, 5);
      assertNull(late.getCachedEffectiveRights());

      // Object carrying newer generation is current and advances session version
      AbstractObject fresh = createObject(session, UserAccessRights.OBJECT_ACCESS_READ, 7);
      assertEquals(Long.valueOf(UserAccessRights.OBJECT_ACCESS_READ), fresh.getCachedEffectiveRights());
      assertEquals(7, session.getObjectRightsVersion().getGeneration());
   }
}
