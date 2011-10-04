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
package org.netxms.client.objecttools;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Detailed information about object tool
 *
 */
public class ObjectToolDetails extends ObjectTool
{
	private boolean modified;
	private List<Long> accessList;
	private List<ObjectToolTableColumn> columns;
	
	/**
	 * Create new tool object
	 * 
	 * @param toolId tool id
	 * @param type tool type
	 * @param name tool name
	 */
	public ObjectToolDetails(long toolId, int type, String name)
	{
		modified = false;
		this.id = toolId;
		this.type = type;
		this.name = name;

		data = "";
		flags = 0;
		description = "";
		snmpOid = "";
		confirmationText = "";
		accessList = new ArrayList<Long>(0);
		columns = new ArrayList<ObjectToolTableColumn>(0);

		createDisplayName();
	}
	
	/**
	 * Create object tool from NXCP message containing detailed tool information.
	 * Intended to be called only by NXCSession methods.
	 * 
	 * @param msg NXCP message
	 */
	public ObjectToolDetails(NXCPMessage msg)
	{
		modified = false;
		id = msg.getVariableAsInt64(NXCPCodes.VID_TOOL_ID);
		name = msg.getVariableAsString(NXCPCodes.VID_NAME);
		type = msg.getVariableAsInteger(NXCPCodes.VID_TOOL_TYPE);
		data = msg.getVariableAsString(NXCPCodes.VID_TOOL_DATA);
		flags = msg.getVariableAsInteger(NXCPCodes.VID_FLAGS);
		description = msg.getVariableAsString(NXCPCodes.VID_DESCRIPTION);
		snmpOid = msg.getVariableAsString(NXCPCodes.VID_TOOL_OID);
		confirmationText = msg.getVariableAsString(NXCPCodes.VID_CONFIRMATION_TEXT);
		
		Long[] acl = msg.getVariableAsUInt32ArrayEx(NXCPCodes.VID_ACL);
		accessList = (acl != null) ? new ArrayList<Long>(Arrays.asList(acl)) : new ArrayList<Long>(0);
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_COLUMNS);
		columns = new ArrayList<ObjectToolTableColumn>(count);
		long varId = NXCPCodes.VID_COLUMN_INFO_BASE;
		for(int i = 0; i < count; i++)
		{
			columns.add(new ObjectToolTableColumn(msg, varId));
			varId += 4;
		}

		createDisplayName();
	}
	
	/**
	 * Fill NXCP message with tool's data.
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(NXCPMessage msg)
	{
		msg.setVariableInt32(NXCPCodes.VID_TOOL_ID, (int)id);
		msg.setVariable(NXCPCodes.VID_NAME, name);
		msg.setVariable(NXCPCodes.VID_DESCRIPTION, description);
		msg.setVariable(NXCPCodes.VID_TOOL_OID, snmpOid);
		msg.setVariable(NXCPCodes.VID_CONFIRMATION_TEXT, confirmationText);
		msg.setVariable(NXCPCodes.VID_TOOL_DATA, data);
		msg.setVariableInt16(NXCPCodes.VID_TOOL_TYPE, type);
		msg.setVariableInt32(NXCPCodes.VID_FLAGS, flags);

		msg.setVariableInt32(NXCPCodes.VID_ACL_SIZE, accessList.size());
		msg.setVariable(NXCPCodes.VID_ACL, accessList.toArray(new Long[accessList.size()]));
		
		msg.setVariableInt16(NXCPCodes.VID_NUM_COLUMNS, columns.size());
		long varId = NXCPCodes.VID_COLUMN_INFO_BASE;
		for(int i = 0; i < columns.size(); i++)
		{
			ObjectToolTableColumn c = columns.get(i);
			msg.setVariable(varId++, c.getName());
			msg.setVariable(varId++, c.getSnmpOid());
			msg.setVariableInt16(varId++, c.getFormat());
			msg.setVariableInt16(varId++, c.getSubstringIndex());
		}
	}

	/**
	 * @return the accessList
	 */
	public List<Long> getAccessList()
	{
		return accessList;
	}

	/**
	 * @return the columns
	 */
	public List<ObjectToolTableColumn> getColumns()
	{
		return columns;
	}

	/**
	 * @param id the id to set
	 */
	public void setId(long id)
	{
		this.id = id;
		modified = true;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
		createDisplayName();
		modified = true;
	}

	/**
	 * @param type the type to set
	 */
	public void setType(int type)
	{
		this.type = type;
		modified = true;
	}

	/**
	 * @param flags the flags to set
	 */
	public void setFlags(int flags)
	{
		this.flags = flags;
		modified = true;
	}

	/**
	 * @param description the description to set
	 */
	public void setDescription(String description)
	{
		this.description = description;
		modified = true;
	}

	/**
	 * @param snmpOid the snmpOid to set
	 */
	public void setSnmpOid(String snmpOid)
	{
		this.snmpOid = snmpOid;
		modified = true;
	}

	/**
	 * @param data the data to set
	 */
	public void setData(String data)
	{
		this.data = data;
		modified = true;
	}

	/**
	 * @param confirmationText the confirmationText to set
	 */
	public void setConfirmationText(String confirmationText)
	{
		this.confirmationText = confirmationText;
		modified = true;
	}

	/**
	 * @return the modified
	 */
	public boolean isModified()
	{
		return modified;
	}

	/**
	 * @param accessList the accessList to set
	 */
	public void setAccessList(List<Long> accessList)
	{
		this.accessList = accessList;
		modified = true;
	}

	/**
	 * @param columns the columns to set
	 */
	public void setColumns(List<ObjectToolTableColumn> columns)
	{
		this.columns = columns;
		modified = true;
	}
}
