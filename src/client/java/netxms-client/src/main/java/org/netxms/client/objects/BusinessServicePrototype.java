/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.client.objects;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Business service representation
 */
public class BusinessServicePrototype extends BusinessService
{
   public static final int IDM_SCRIPT = 0;
   public static final int IDM_AGENT_LIST = 1;
   public static final int IDM_AGENT_TABLE = 2;
   
   protected String instance;
   protected int instanceDiscoveryMethod;
   protected String instanceDiscoveryData;
   protected String instanceDiscoveryFilter;
   protected long sourceNode;
   

   /**
    * Constructor
    * 
    * @param msg NXCPMessage with data
    * @param session NXCPSession 
    */
	public BusinessServicePrototype(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
      instance = msg.getFieldAsString(NXCPCodes.VID_INSTANCE);
      instanceDiscoveryMethod = msg.getFieldAsInt32(NXCPCodes.VID_INSTD_METHOD);
      instanceDiscoveryData = msg.getFieldAsString(NXCPCodes.VID_INSTD_DATA);
      instanceDiscoveryFilter = msg.getFieldAsString(NXCPCodes.VID_INSTD_FILTER); 
      sourceNode = msg.getFieldAsInt32(NXCPCodes.VID_NODE_ID);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "BusinessServicePrototype";
	}

   /**
    * @return the instance
    */
   public String getInstance()
   {
      return instance;
   }

   /**
    * @return the instanceDiscoveryMethod
    */
   public int getInstanceDiscoveryMethod()
   {
      return instanceDiscoveryMethod;
   }

   /**
    * @return the instanceDiscoveryData
    */
   public String getInstanceDiscoveryData()
   {
      return instanceDiscoveryData;
   }

   /**
    * @return the instanceDiscoveryFilter
    */
   public String getInstanceDiscoveryFilter()
   {
      return instanceDiscoveryFilter;
   }

   public long getSourceNode()
   {
      return sourceNode;
   }
}
