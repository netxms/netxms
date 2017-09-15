/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.client.snmp;

import org.netxms.base.NXCPMessage;

/**
 * SNMP trap parameter mapping
 */
public class SnmpTrapParameterMapping
{
	public static final int BY_OBJECT_ID = 0;
	public static final int BY_POSITION = 1;
	
	// Flags
	public static final int FORCE_TEXT = 0x0001;
	
	private int type;
	private int flags;
	private SnmpObjectId objectId;
	private int position;
	private String description;
	
	/**
	 * Create mapping by position
	 * 
	 * @param position parameter's position
	 */
	public SnmpTrapParameterMapping(int position)
	{
		type = BY_POSITION;
		this.position = position;
	}

	/**
	 * Create mapping by object ID
	 * 
	 * @param oid SNMP object ID
	 */
	public SnmpTrapParameterMapping(SnmpObjectId oid)
	{
		type = BY_OBJECT_ID;
		objectId = oid;
	}
	
   /**
    * Create mapping from NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public SnmpTrapParameterMapping(NXCPMessage msg, long baseId)
   {
      type = msg.getFieldAsInt32(baseId);
      if (type == BY_POSITION)
         position = (int)msg.getFieldAsInt64(baseId + 1);
      else
         objectId = new SnmpObjectId(msg.getFieldAsUInt32Array(baseId + 1));
      
      description = msg.getFieldAsString(baseId + 2);
      flags = msg.getFieldAsInt32(baseId + 3);
   }

	/**
	 * Fill NXCP message with parameter mapping's data
	 * 
	 * @param msg NXCP message
	 * @param baseId base field ID
	 */
	public void fillMessage(NXCPMessage msg, long baseId)
	{
      msg.setFieldInt32(baseId, flags);
      msg.setField(baseId + 1, description);
      msg.setFieldInt32(baseId + 2, type);
		if (type == BY_POSITION)
		{
			msg.setFieldInt32(baseId + 3, position);
		}
		else
		{
         objectId.setNXCPVariable(msg, baseId + 3);
			msg.setFieldInt32(baseId + 4, objectId.getLength());
		}
	}

	/**
	 * Get mapping type
	 * 
	 * @return mapping type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * Set mapping type
	 * 
	 * @param type new mapping type
	 */
	public void setType(int type)
	{
		this.type = type;
	}

	/**
	 * Get SNMP object ID
	 * 
	 * @return SNMP OID
	 */
	public SnmpObjectId getObjectId()
	{
		return objectId;
	}

	/**
	 * Set SNMP object ID
	 * 
	 * @param objectId SNMP OID
	 */
	public void setObjectId(SnmpObjectId objectId)
	{
		this.objectId = objectId;
	}

	/**
	 * Get position number
	 * 
	 * @return position number
	 */
	public int getPosition()
	{
		return position;
	}

	/**
	 * Set position number (first position is 1).
	 * 
	 * @param position new position number
	 */
	public void setPosition(int position)
	{
		this.position = position;
	}

	/**
	 * Get description
	 * 
	 * @return description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * Set description
	 * 
	 * @param description new description
	 */
	public void setDescription(String description)
	{
		this.description = description;
	}

	/**
	 * Get flags
	 * 
	 * @return flags (bit mask)
	 */
	public int getFlags()
	{
		return flags;
	}

	/**
	 * Set flags
	 * 
	 * @param flags new flags
	 */
	public void setFlags(int flags)
	{
		this.flags = flags;
	}
}
