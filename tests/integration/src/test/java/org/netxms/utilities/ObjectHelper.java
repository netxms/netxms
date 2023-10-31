/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.utilities;

import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.PollState;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.tests.TestConstants;

public class ObjectHelper

{

   /**
    * Find management node
    * 
    * @param session NXCSession object
    * @return management node
    * @throws IOException
    * @throws NXCException
    */
   public static AbstractObject findManagementServer(NXCSession session) throws IOException, NXCException
   {
      session.syncObjects();
      for(AbstractObject object : session.getAllObjects())
      {
         if ((object instanceof Node) && ((Node)object).isManagementServer())
         {
            return object;
         }
      }
      return null;

   }

   /**
    * Get test node with topology
    * 
    * @param session NXCSession object
    * @return test node which is used for methods: testLinkLayerTopology, testRoutingTable, testSwitchForwardingTable
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static AbstractObject getTopologyNode(NXCSession session) throws IOException, NXCException, InterruptedException
   {
      session.syncObjects();
      ObjectHelper.checkObjectCreated(session);

      for(AbstractObject object : session.getAllObjects())
      {
         if ((object instanceof Node) && ((Node)object).getPrimaryIP().getHostAddress().equals(TestConstants.TEST_NODE_2))
         {
            return object;
         }
      }
      return null;
   }

   /**
    * Checks test node is created, if not create test node
    * 
    * @param session
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static void checkObjectCreated(NXCSession session) throws IOException, NXCException, InterruptedException
   {
      session.syncObjects();
      String[] nodes = { TestConstants.TEST_NODE_1, TestConstants.TEST_NODE_2, TestConstants.TEST_NODE_3 };
      // checking if the object is created, and if it's not, create it
      for(String nodeIP : nodes)
      {
         boolean found = false;

         for(AbstractObject object : session.getAllObjects())
         {
            if ((object instanceof Node) && (((Node)object).getPrimaryIP().getHostAddress().equals(nodeIP)))
            {
               found = true;
               break;
            }
         }

         if (!found)
         {
            NXCObjectCreationData cd = new NXCObjectCreationData(GenericObject.OBJECT_NODE, nodeIP, GenericObject.SERVICEROOT);
            cd.setPrimaryName(nodeIP);
            session.createObjectSync(cd);

         }

      }
      // checking that the topology poll passed for all nodes
      for(String nodeIP : nodes)
      {
         boolean pending = true;
         for(AbstractObject object : session.getAllObjects())
         {
            if ((object instanceof Node) && (((Node)object).getPrimaryIP().getHostAddress().equals(nodeIP)))
            {
               do
               {
                  Thread.sleep(1000);
                  Node findedNode = (Node)session.findObjectById(object.getObjectId());
                  for(PollState status : findedNode.getPollStates())
                  {
                     if ("topology".equals(status.getPollType()))
                     {
                        Date lastCompleted = status.getLastCompleted();
                        pending = (lastCompleted.getTime() == 0) || status.isPending();
                     }

                  }
               } while(pending);
               break;
            }

         }

      }
   }

}
