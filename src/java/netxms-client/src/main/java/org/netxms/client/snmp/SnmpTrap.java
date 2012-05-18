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
package org.netxms.client.snmp;

import java.util.ArrayList;
import java.util.List;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Object of this class represents single SNMP trap matching configuration record
 *
 */
public class SnmpTrap
{
	private long id;
	private String description;
	private SnmpObjectId objectId;
	private int eventCode;
	private String userTag;
	private List<SnmpTrapParameterMapping> parameterMapping;
	
	/**
	 * Default constructor
	 */
	public SnmpTrap()
	{
		parameterMapping = new ArrayList<SnmpTrapParameterMapping>();
	}
	
	/**
	 * Create SNMP trap configuration object from NXCP message
	 * 
	 * @param msg NXCP message
	 */
	public SnmpTrap(final NXCPMessage msg)
	{
		id = msg.getVariableAsInt64(NXCPCodes.VID_TRAP_ID);
		description = msg.getVariableAsString(NXCPCodes.VID_DESCRIPTION);
		objectId = new SnmpObjectId(msg.getVariableAsUInt32Array(NXCPCodes.VID_TRAP_OID));
		eventCode = msg.getVariableAsInteger(NXCPCodes.VID_EVENT_CODE);
		userTag = msg.getVariableAsString(NXCPCodes.VID_USER_TAG);
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_TRAP_NUM_MAPS);
		parameterMapping = new ArrayList<SnmpTrapParameterMapping>(count);
		for(int i = 0; i < count; i++)
		{
			SnmpTrapParameterMapping pm;
			long oidLen = msg.getVariableAsInt64(NXCPCodes.VID_TRAP_PLEN_BASE + i);
			if ((oidLen & 0x80000000) == 0)
			{
				// mapping by OID
				pm = new SnmpTrapParameterMapping(new SnmpObjectId(msg.getVariableAsUInt32Array(NXCPCodes.VID_TRAP_PNAME_BASE + i)));
			}
			else
			{
				// mapping by position
				pm = new SnmpTrapParameterMapping((int)(oidLen & 0x7FFFFFFF));
			}
			pm.setDescription(msg.getVariableAsString(NXCPCodes.VID_TRAP_PDESCR_BASE + i));
			pm.setFlags(msg.getVariableAsInteger(NXCPCodes.VID_TRAP_PFLAGS_BASE + i));
			parameterMapping.add(pm);
		}
	}
	
	/**
	 * Create SNMP trap configuration object from summary NXCP message
	 * 
	 * @param msg NXCP message
	 */
	public SnmpTrap(NXCPMessage msg, long baseId)
	{
		id = msg.getVariableAsInt64(baseId);
		description = msg.getVariableAsString(baseId + 4);
		objectId = new SnmpObjectId(msg.getVariableAsUInt32Array(baseId + 2));
		eventCode = msg.getVariableAsInteger(baseId + 3);
		parameterMapping = new ArrayList<SnmpTrapParameterMapping>(0);
	}
	
	/**
	 * Fill NXCP message with trap configuration data.
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(NXCPMessage msg)
	{
		msg.setVariableInt32(NXCPCodes.VID_TRAP_ID, (int)id);
		msg.setVariableInt32(NXCPCodes.VID_EVENT_CODE, eventCode);
		msg.setVariable(NXCPCodes.VID_DESCRIPTION, description);
		msg.setVariable(NXCPCodes.VID_USER_TAG, userTag);
		msg.setVariableInt32(NXCPCodes.VID_TRAP_OID_LEN, objectId.getLength());
		objectId.setNXCPVariable(msg, NXCPCodes.VID_TRAP_OID);
		msg.setVariableInt32(NXCPCodes.VID_TRAP_NUM_MAPS, parameterMapping.size());
		for(int i = 0; i < parameterMapping.size(); i++)
		{
			parameterMapping.get(i).fillMessage(msg, i);
		}
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @param id the id to set
	 */
	public void setId(long id)
	{
		this.id = id;
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
	 * @return the eventCode
	 */
	public int getEventCode()
	{
		return eventCode;
	}

	/**
	 * @param eventCode the eventCode to set
	 */
	public void setEventCode(int eventCode)
	{
		this.eventCode = eventCode;
	}

	/**
	 * @return the userTag
	 */
	public String getUserTag()
	{
		return userTag;
	}

	/**
	 * @param userTag the userTag to set
	 */
	public void setUserTag(String userTag)
	{
		this.userTag = userTag;
	}

	/**
	 * @return the parameterMapping
	 */
	public List<SnmpTrapParameterMapping> getParameterMapping()
	{
		return parameterMapping;
	}
}
