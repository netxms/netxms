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
package org.netxms.api.client.mt;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Mapping table
 */
public class MappingTable
{
	/**
	 * Flag which indicates that mapping table contain numeric keys.
	 */
	public static final int NUMERIC_KEYS = 0x00000001;
	
	private int id;
	private String name;
	private String description;
	private int flags;
	private List<MappingTableEntry> data;
	
	/**
	 * Create new empty mapping table with ID 0.
	 * 
	 * @param name
	 * @param description
	 */
	public MappingTable(String name, String description)
	{
		this.id = 0;
		this.name = name;
		this.description = description;
		this.flags = 0;
		this.data = new ArrayList<MappingTableEntry>(0);
	}
	
	/**
	 * Create mapping table object from NXCP message
	 * 
	 * @param msg NXCP message
	 */
	public MappingTable(NXCPMessage msg)
	{
		id = msg.getVariableAsInteger(NXCPCodes.VID_MAPPING_TABLE_ID);
		name = msg.getVariableAsString(NXCPCodes.VID_NAME);
		description = msg.getVariableAsString(NXCPCodes.VID_DESCRIPTION);
		flags = msg.getVariableAsInteger(NXCPCodes.VID_FLAGS);
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_ELEMENTS);
		data = new ArrayList<MappingTableEntry>(count);
		long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			data.add(new MappingTableEntry(msg, varId));
			varId += 10;
		}
	}
	
	/**
	 * Fill NXCP message with table's data
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(NXCPMessage msg)
	{
		msg.setVariableInt32(NXCPCodes.VID_MAPPING_TABLE_ID, id);
		msg.setVariable(NXCPCodes.VID_NAME, name);
		msg.setVariable(NXCPCodes.VID_DESCRIPTION, description);
		msg.setVariableInt32(NXCPCodes.VID_FLAGS, flags);
		
		msg.setVariableInt32(NXCPCodes.VID_NUM_ELEMENTS, data.size());
		long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
		for(MappingTableEntry e : data)
		{
			msg.setVariable(varId++, e.getKey());
			msg.setVariable(varId++, e.getValue());
			msg.setVariable(varId++, e.getDescription());
			varId += 7;
		}
	}

	/**
	 * @return the name
	 */
	public final String getName()
	{
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public final void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return the description
	 */
	public final String getDescription()
	{
		return description;
	}

	/**
	 * @param description the description to set
	 */
	public final void setDescription(String description)
	{
		this.description = description;
	}

	/**
	 * @return the flags
	 */
	public final int getFlags()
	{
		return flags;
	}

	/**
	 * @param flags the flags to set
	 */
	public final void setFlags(int flags)
	{
		this.flags = flags;
	}

	/**
	 * @return the id
	 */
	public final int getId()
	{
		return id;
	}

	/**
	 * @return the data
	 */
	public final List<MappingTableEntry> getData()
	{
		return data;
	}
}
