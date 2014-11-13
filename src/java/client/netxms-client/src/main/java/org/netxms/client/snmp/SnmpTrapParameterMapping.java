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

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * SNMP trap parameter mapping
 *
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
	 * Fill NXCP message with parameter mapping's data
	 * 
	 * @param index mapping index
	 */
	public void fillMessage(NXCPMessage msg, int index)
	{
		msg.setVariableInt32(NXCPCodes.VID_TRAP_PFLAGS_BASE + index, flags);
		msg.setVariable(NXCPCodes.VID_TRAP_PDESCR_BASE + index, description);
		if (type == BY_POSITION)
		{
			msg.setVariableInt32(NXCPCodes.VID_TRAP_PLEN_BASE + index, position | 0x80000000);
		}
		else
		{
			msg.setVariableInt32(NXCPCodes.VID_TRAP_PLEN_BASE + index, objectId.getLength());
			objectId.setNXCPVariable(msg, NXCPCodes.VID_TRAP_PNAME_BASE + index);
		}
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @param type the type to set
	 */
	public void setType(int type)
	{
		this.type = type;
	}

	/**
	 * @return the objectId
	 */
	public SnmpObjectId getObjectId()
	{
		return objectId;
	}

	/**
	 * @param objectId the objectId to set
	 */
	public void setObjectId(SnmpObjectId objectId)
	{
		this.objectId = objectId;
	}

	/**
	 * @return the position
	 */
	public int getPosition()
	{
		return position;
	}

	/**
	 * @param position the position to set
	 */
	public void setPosition(int position)
	{
		this.position = position;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @param description the description to set
	 */
	public void setDescription(String description)
	{
		this.description = description;
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

	/**
	 * @param flags the flags to set
	 */
	public void setFlags(int flags)
	{
		this.flags = flags;
	}
}
