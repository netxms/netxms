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
package org.netxms.client.objects;

import java.util.Set;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Business service representation
 */
public class BusinessServicePrototype extends BaseBusinessService
{
   private int instanceDiscoveryMethod;
   private String instanceDiscoveryData;
   private String instanceDiscoveryFilter;
   private long sourceNode;

   /**
    * Constructor
    * 
    * @param msg NXCPMessage with data
    * @param session NXCPSession 
    */
	public BusinessServicePrototype(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
      instanceDiscoveryMethod = msg.getFieldAsInt32(NXCPCodes.VID_INSTD_METHOD);
      instanceDiscoveryData = msg.getFieldAsString(NXCPCodes.VID_INSTD_DATA);
      instanceDiscoveryFilter = msg.getFieldAsString(NXCPCodes.VID_INSTD_FILTER); 
      sourceNode = msg.getFieldAsInt32(NXCPCodes.VID_NODE_ID);
	}

   /**
    * @see org.netxms.client.objects.GenericObject#getObjectClassName()
    */
	@Override
	public String getObjectClassName()
	{
		return "BusinessServicePrototype";
	}

   /**
    * Get instance discovery method.
    *
    * @return instance discovery method
    */
   public int getInstanceDiscoveryMethod()
   {
      return instanceDiscoveryMethod;
   }

   /**
    * Get instance discovery data (specific to selected method).
    *
    * @return instance discovery data
    */
   public String getInstanceDiscoveryData()
   {
      return instanceDiscoveryData;
   }

   /**
    * Get instance discovery filter.
    *
    * @return instance discovery filter
    */
   public String getInstanceDiscoveryFilter()
   {
      return instanceDiscoveryFilter;
   }

   /**
    * Get source node (node to get instances from).
    *
    * @return source node ID or 0 if none set
    */
   public long getSourceNode()
   {
      return sourceNode;
   }

   /**
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, instanceDiscoveryData);
      addString(strings, instanceDiscoveryFilter);
      return strings;
   }
}
