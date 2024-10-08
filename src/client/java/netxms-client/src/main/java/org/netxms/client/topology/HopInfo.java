/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
 * Information about single hop in network path
 */
public class HopInfo
{
   public static final int ROUTE = 0;
   public static final int VPN = 1;
   public static final int PROXY = 2;
   public static final int DUMMY = 3;

   private int index;
	private long nodeId;
	private InetAddress nextHop;
	private int ifIndex;
   private int type;
	private String name;

   /**
    * Create "invalid" hop info object
    *
    * @param index hop index
    */
   public HopInfo(int index)
   {
      this.index = index;
      nodeId = -1;
      nextHop = null;
      ifIndex = 0;
      type = DUMMY;
      name = "";
   }

	/**
    * Create hop info object from NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base variable ID
    * @param index hop index
    */
   protected HopInfo(NXCPMessage msg, long baseId, int index)
	{
      this.index = index;
		nodeId = msg.getFieldAsInt64(baseId);
		nextHop = msg.getFieldAsInetAddress(baseId + 1);
		ifIndex = msg.getFieldAsInt32(baseId + 2);
      type = msg.getFieldAsInt32(baseId + 3);
		name = msg.getFieldAsString(baseId + 4);
	}

	/**
    * @return the index
    */
   public int getIndex()
   {
      return index;
   }

   /**
    * @return the nodeId
    */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @return the nextHop
	 */
	public InetAddress getNextHop()
	{
		return nextHop;
	}

	/**
	 * @return the ifIndex
	 */
	public int getIfIndex()
	{
		return ifIndex;
	}

	/**
	 * @return the isVpn
	 */
   public int getType()
	{
      return type;
	}

   /**
    * @return the name
    */
   public String getName()
   {
      return (name != null) ? name : "";
   }
}
