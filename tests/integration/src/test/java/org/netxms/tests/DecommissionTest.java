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
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.Date;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.TextOutputListener;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.GenericObject;
import org.netxms.utilities.TestHelper;

/**
 * Tests that a decommissioned node cannot be returned to managed state by any server-side path:
 * NXSL method manage(), NXSL function ManageObject(), client API, and managing a parent object.
 */
public class DecommissionTest extends AbstractSessionTest
{
   private static final String MANAGE_SCRIPT = "println($node->manage());\nprintln(ManageObject($node));\n";

   private NXCSession session;

   private long createUnmanagedObject(int objectClass, String name) throws Exception
   {
      NXCObjectCreationData cd = new NXCObjectCreationData(objectClass, name, GenericObject.SERVICEROOT);
      cd.setCreationFlags(NXCObjectCreationData.CF_CREATE_UNMANAGED);
      return session.createObjectSync(cd).getObjectId();
   }

   private ObjectStatus statusOf(long objectId) throws Exception
   {
      session.syncObjects();
      return session.findObjectById(objectId).getStatus();
   }

   private String runScript(long objectId, String script) throws Exception
   {
      final StringBuilder output = new StringBuilder();
      session.executeScript(objectId, script, (String)null, new TextOutputListener() {
         @Override
         public void messageReceived(String text)
         {
            output.append(text);
         }

         @Override
         public void setStreamId(long streamId)
         {
         }

         @Override
         public void onSuccess()
         {
         }

         @Override
         public void onFailure(Exception exception)
         {
            output.append("EXECUTION ERROR: ").append(exception.getMessage());
         }
      });
      return output.toString();
   }

   @Test
   public void testDecommissionedNodeStaysUnmanaged() throws Exception
   {
      session = connectAndLogin();
      session.syncObjects();

      long decommissionedNodeId = 0;
      long normalNodeId = 0;
      long containerId = 0;
      try
      {
         decommissionedNodeId = createUnmanagedObject(GenericObject.OBJECT_NODE, "DecommissionTest-A");
         normalNodeId = createUnmanagedObject(GenericObject.OBJECT_NODE, "DecommissionTest-B");

         session.decommissionNode(decommissionedNodeId, new Date(System.currentTimeMillis() + 86400000L), false);
         assertEquals(ObjectStatus.UNMANAGED, statusOf(decommissionedNodeId));

         // NXSL: NetObj::manage() and ManageObject() must refuse and return false
         assertTrue(runScript(decommissionedNodeId, MANAGE_SCRIPT).replace("\r", "").startsWith("false\nfalse"));
         assertEquals(ObjectStatus.UNMANAGED, statusOf(decommissionedNodeId));

         // Client API: manage request must be rejected
         final long nodeId = decommissionedNodeId;
         TestHelper.assertRcc(RCC.INCOMPATIBLE_OPERATION, () -> session.setObjectManaged(nodeId, true));
         assertEquals(ObjectStatus.UNMANAGED, statusOf(decommissionedNodeId));

         // Managing a parent must manage normal children but skip the decommissioned node
         containerId = session.createObjectSync(new NXCObjectCreationData(GenericObject.OBJECT_CONTAINER, "DecommissionTest-Container", GenericObject.SERVICEROOT)).getObjectId();
         session.bindObject(containerId, decommissionedNodeId);
         session.bindObject(containerId, normalNodeId);
         session.setObjectManaged(containerId, false);
         assertEquals(ObjectStatus.UNMANAGED, statusOf(containerId));
         session.setObjectManaged(containerId, true);
         assertNotEquals(ObjectStatus.UNMANAGED, statusOf(containerId));
         assertNotEquals(ObjectStatus.UNMANAGED, statusOf(normalNodeId));
         assertEquals(ObjectStatus.UNMANAGED, statusOf(decommissionedNodeId));

         // Unmanaging a decommissioned node is still allowed
         session.setObjectManaged(decommissionedNodeId, false);
         assertEquals(ObjectStatus.UNMANAGED, statusOf(decommissionedNodeId));
      }
      finally
      {
         if (containerId != 0)
            session.deleteObject(containerId);
         if (normalNodeId != 0)
            session.deleteObject(normalNodeId);
         if (decommissionedNodeId != 0)
            session.deleteObject(decommissionedNodeId);
      }
   }
}
