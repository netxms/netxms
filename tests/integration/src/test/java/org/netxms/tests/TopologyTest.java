/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import static org.junit.jupiter.api.Assertions.assertNotNull;
import java.io.IOException;
import java.util.List;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Subnet;
import org.netxms.client.topology.FdbEntry;
import org.netxms.client.topology.Route;
import org.netxms.utilities.TestHelper;


/**
 * Tests for network topology functions
 */
public class TopologyTest extends AbstractSessionTest
{   
   public static AbstractObject findSubnet(NXCSession session) throws IOException, NXCException, InterruptedException
   {
      Node node = TestHelper.findManagementServer(session);
      assertNotNull(node);
      session.syncObjects();
      for(AbstractObject parent : node.getParentsAsArray())
      {
         if (parent instanceof Subnet) 
         {
            return parent;
         }
      }
      return null;
   }

   @Test
   public void testAddressMap() throws Exception
   {
      final NXCSession session = connectAndLogin();

      AbstractNode node = (AbstractNode)TestHelper.getTopologyNode(session);
      assertNotNull(node);
      System.out.println("parent " + TopologyTest.findSubnet(session));
      long[] map = session.getSubnetAddressMap(TopologyTest.findSubnet(session).getObjectId());
      for(int i = 0; i < map.length; i++)
         System.out.println(i + ": " + map[i]);
      
      session.disconnect();
   }
   
   @Test
   public void testLinkLayerTopology() throws Exception
   {
      final NXCSession session = connectAndLogin();     
      Node node = TestHelper.getTopologyNode(session);
      assertNotNull(node);

      NetworkMapPage page = session.queryLayer2Topology(node.getObjectId(), -1, false);
      for(NetworkMapElement e : page.getElements())
         System.out.println(e.toString());
      for(NetworkMapLink l : page.getLinks())
         System.out.println(l.toString());
      
      session.disconnect();
   }

   @Test
   public void testRoutingTable() throws Exception
   {
      final NXCSession session = connectAndLogin();
      Node node = TestHelper.getTopologyNode(session);
      assertNotNull(node);

      List<Route> rt = session.getRoutingTable(node.getObjectId());
      for(Route r : rt)
         System.out.println(r.toString());
      
      session.disconnect();
   }
     
   @Test
   public void testSwitchForwardingTable() throws Exception
   {
      final NXCSession session = connectAndLogin();
      Node node = TestHelper.getTopologyNode(session);
      assertNotNull(node);

      List<FdbEntry> fdb = session.getSwitchForwardingDatabase(node.getObjectId());
      for(FdbEntry e : fdb)
         System.out.println(e.toString());
      
      session.disconnect();
   }
}
