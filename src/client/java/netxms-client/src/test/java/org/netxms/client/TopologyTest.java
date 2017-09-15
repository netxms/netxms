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
package org.netxms.client;

import java.util.List;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.topology.FdbEntry;
import org.netxms.client.topology.Route;


/**
 * Tests for network topology functions
 */
public class TopologyTest extends AbstractSessionTest
{
   
   public void testAddressMap() throws Exception
   {
      final NXCSession session = connect();

      long[] map = session.getSubnetAddressMap(TestConstants.SUBNET_ID);
      for(int i = 0; i < map.length; i++)
         System.out.println(i + ": " + map[i]);
      
      session.disconnect();
   }

   public void testLinkLayerTopology() throws Exception
   {
      final NXCSession session = connect();

      NetworkMapPage page = session.queryLayer2Topology(TestConstants.NODE_ID);
      for(NetworkMapElement e : page.getElements())
         System.out.println(e.toString());
      for(NetworkMapLink l : page.getLinks())
         System.out.println(l.toString());
      
      session.disconnect();
   }

   public void testRoutingTable() throws Exception
   {
      final NXCSession session = connect();

      List<Route> rt = session.getRoutingTable(TestConstants.NODE_ID);
      for(Route r : rt)
         System.out.println(r.toString());
      
      session.disconnect();
   }

   public void testSwitchForwardingTable() throws Exception
   {
      final NXCSession session = connect();

      List<FdbEntry> fdb = session.getSwitchForwardingDatabase(TestConstants.NODE_ID);
      for(FdbEntry e : fdb)
         System.out.println(e.toString());
      
      session.disconnect();
   }
}
