/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.client.topology;

import java.net.InetAddress;
import org.netxms.base.NXCPMessage;

/**
 * OSPF area information
 */
public class OSPFArea
{
   private InetAddress id;
   private int lsaCount;
   private int areaBorderRouterCount;
   private int asBorderRouterCount;

   /**
    * Create area object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public OSPFArea(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInetAddress(baseId);
      lsaCount = msg.getFieldAsInt32(baseId + 1);
      areaBorderRouterCount = msg.getFieldAsInt32(baseId + 2);
      asBorderRouterCount = msg.getFieldAsInt32(baseId + 3);
   }

   /**
    * Get area ID.
    *
    * @return area ID
    */
   public InetAddress getId()
   {
      return id;
   }

   /**
    * Get number of link state advertisements (LSA).
    * 
    * @return number of link state advertisements
    */
   public int getLsaCount()
   {
      return lsaCount;
   }

   /**
    * Get number of area border routers.
    *
    * @return number of area border routers
    */
   public int getAreaBorderRouterCount()
   {
      return areaBorderRouterCount;
   }

   /**
    * Get number of autonomous system border routers.
    *
    * @return number of autonomous system border routers
    */
   public int getAsBorderRouterCount()
   {
      return asBorderRouterCount;
   }
}
