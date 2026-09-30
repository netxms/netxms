/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
 * <p/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p/>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p/>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertTrue;
import java.net.InetAddress;
import org.junit.jupiter.api.Test;
import org.netxms.base.InetAddressEx;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.GenericObject;

/**
 * Tests for chassis binding under its controller node
 */
public class ChassisControllerBindingTest extends AbstractSessionTest
{
   private static final long WAIT_TIMEOUT = 30000;

   private static long createTestNode(NXCSession session, String name, String address) throws Exception
   {
      NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, name, GenericObject.SERVICEROOT);
      cd.setCreationFlags(NXCObjectCreationData.CF_CREATE_UNMANAGED);
      cd.setIpAddress(new InetAddressEx(InetAddress.getByName(address), 0));
      return session.createObjectSync(cd).getObjectId();
   }

   private static boolean contains(long[] list, long id)
   {
      for(long v : list)
         if (v == id)
            return true;
      return false;
   }

   private static void waitForParents(NXCSession session, long objectId, long expectedParent, long unexpectedParent) throws Exception
   {
      long deadline = System.currentTimeMillis() + WAIT_TIMEOUT;
      while(true)
      {
         long[] parents = session.findObjectById(objectId).getParentIdList();
         if (contains(parents, expectedParent) && !contains(parents, unexpectedParent))
            return;
         assertTrue(System.currentTimeMillis() < deadline,
               "Object " + objectId + " is not under " + expectedParent + " only (unexpected " + unexpectedParent + ") within timeout");
         Thread.sleep(200);
      }
   }

   private static void deleteIfExists(NXCSession session, long objectId)
   {
      try
      {
         if ((objectId != 0) && (session.findObjectById(objectId) != null))
            session.deleteObject(objectId);
      }
      catch(Exception e)
      {
         System.out.println("Cleanup: cannot delete object " + objectId + ": " + e.getMessage());
      }
   }

   /**
    * Changing only the controller id (no flag mask in the same request, as a plain API client would send it)
    * must move a chassis bound under its controller to the new controller.
    */
   @Test
   public void testControllerChangeWithoutFlagsRebinds() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      String suffix = Long.toString(System.currentTimeMillis());
      long controllerA = 0;
      long controllerB = 0;
      long chassisId = 0;
      try
      {
         controllerA = createTestNode(session, "chassis-controller-a-" + suffix, "192.0.2.201");
         controllerB = createTestNode(session, "chassis-controller-b-" + suffix, "192.0.2.202");

         NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_CHASSIS, "chassis-" + suffix, GenericObject.SERVICEROOT);
         cd.setControllerId(controllerA);
         chassisId = session.createObjectSync(cd).getObjectId();

         NXCObjectModificationData md = new NXCObjectModificationData(chassisId);
         md.setObjectFlags(Chassis.CHF_BIND_UNDER_CONTROLLER, Chassis.CHF_BIND_UNDER_CONTROLLER);
         session.modifyObject(md);
         waitForParents(session, chassisId, controllerA, controllerB);

         md = new NXCObjectModificationData(chassisId);
         md.setControllerId(controllerB);
         session.modifyObject(md);
         waitForParents(session, chassisId, controllerB, controllerA);
      }
      finally
      {
         deleteIfExists(session, chassisId);
         deleteIfExists(session, controllerA);
         deleteIfExists(session, controllerB);
      }
   }
}
