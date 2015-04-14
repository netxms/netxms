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
package org.netxms.client.mt;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCommon;

/**
 * Mapping table
 */
public class MappingTable
{
	/**
	 * Flag which indicates that mapping table contain numeric keys.
	 */
	public static final int NUMERIC_KEYS = 0x00000001;
	
	protected int id;
	protected UUID guid;
	protected String name;
	protected String description;
	protected int flags;
	protected List<MappingTableEntry> data;
	protected Map<String, String> hashMap = null;
	
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
		this.guid = NXCommon.EMPTY_GUID;
		this.data = new ArrayList<MappingTableEntry>(0);
	}
	
	/**
	 * Create mapping table object from NXCP message
	 * 
	 * @param msg NXCP message
	 */
	public MappingTable(NXCPMessage msg)
	{
		id = msg.getFieldAsInt32(NXCPCodes.VID_MAPPING_TABLE_ID);
		name = msg.getFieldAsString(NXCPCodes.VID_NAME);
		description = msg.getFieldAsString(NXCPCodes.VID_DESCRIPTION);
		flags = msg.getFieldAsInt32(NXCPCodes.VID_FLAGS);
      guid = msg.getFieldAsUUID(NXCPCodes.VID_GUID);
		
		int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
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
		msg.setFieldInt32(NXCPCodes.VID_MAPPING_TABLE_ID, id);
		msg.setField(NXCPCodes.VID_NAME, name);
		msg.setField(NXCPCodes.VID_DESCRIPTION, description);
		msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
		if (guid != null)
		   msg.setField(NXCPCodes.VID_GUID, guid);
		
		msg.setFieldInt32(NXCPCodes.VID_NUM_ELEMENTS, data.size());
		long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
		for(MappingTableEntry e : data)
		{
			msg.setField(varId++, e.getKey());
			msg.setField(varId++, e.getValue());
			msg.setField(varId++, e.getDescription());
			varId += 7;
		}
	}
	
	/**
	 * @param key
	 * @return
	 */
	public String lookup(String key)
	{
		if (hashMap == null)
			buildHash();
		return hashMap.get(key);
	}
	
	/**
	 * Build has for fast lookup
	 */
	public void buildHash()
	{
		hashMap = new HashMap<String, String>(data.size());
		for(MappingTableEntry e : data)
			hashMap.put(e.getKey(), e.getValue());
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

   /**
    * @return the guid
    */
   public UUID getGuid()
   {
      return guid;
   }
}
