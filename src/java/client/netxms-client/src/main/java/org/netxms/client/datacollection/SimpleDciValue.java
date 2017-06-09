/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.client.datacollection;

import org.netxms.base.NXCPMessage;

/**
 * DCI value for simple (single-valued) DCI
 */
public class SimpleDciValue extends DciValue
{
	/**
	 * @param nodeId node id
	 * @param msg source message
	 * @param base Base field ID for value object
	 */
	protected SimpleDciValue(long nodeId, NXCPMessage msg, long base)
	{
		super(nodeId, msg, base);
	}
	
	/**
    * @param id node id
    * @param value value
    * @param dataType type
    * @param status DCI status
    */
   public SimpleDciValue(NXCPMessage msg, long base)
   {
      super();
      this.id = msg.getFieldAsInt64(base + 1);
      this.value = msg.getFieldAsString(base + 2);
      this.dataType = msg.getFieldAsInt32(base + 3);
      this.status = msg.getFieldAsInt32(base + 4);
      this.nodeId = msg.getFieldAsInt64(base + 5);
      this.source = msg.getFieldAsInt32(base + 6);
      this.name = msg.getFieldAsString(base + 7);
      this.description = msg.getFieldAsString(base + 8);
   }
}
