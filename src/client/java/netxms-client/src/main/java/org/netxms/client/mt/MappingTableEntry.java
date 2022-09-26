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
package org.netxms.client.mt;

import org.netxms.base.NXCPMessage;

/**
 * Entry in mapping table
 */
public class MappingTableEntry
{
	private String key;
	private String value;
	private String description;
	
	/**
	 * Create new entry.
	 *
	 * @param key key
	 * @param value value
	 * @param description description
	 */
	public MappingTableEntry(String key, String value, String description)
	{
		this.key = key;
		this.value = value;
		this.description = description;
	}

	/**
	 * Create entry from NXCP message.
	 * 
	 * @param msg NXCP message
	 * @param baseId base field ID
	 */
	protected MappingTableEntry(NXCPMessage msg, long baseId)
	{
		key = msg.getFieldAsString(baseId);
		value = msg.getFieldAsString(baseId + 1);
		description = msg.getFieldAsString(baseId + 2);
	}

	/**
	 * @return the key
	 */
   public String getKey()
	{
		return key;
	}

	/**
	 * @return the value
	 */
   public String getValue()
	{
		return value;
	}

	/**
	 * @return the description
	 */
   public String getDescription()
	{
		return description;
	}

	/**
	 * @param key the key to set
	 */
   public void setKey(String key)
	{
		this.key = key;
	}

	/**
	 * @param value the value to set
	 */
   public void setValue(String value)
	{
		this.value = value;
	}

	/**
	 * @param description the description to set
	 */
   public void setDescription(String description)
	{
		this.description = description;
	}

   /**
    * Check if this entry is an empty one.
    *
    * @return true if this entry is an empty one
    */
   public boolean isEmpty()
   {
      return ((key == null) || key.isEmpty()) && ((value == null) || value.isEmpty()) && ((description == null) || description.isEmpty());
   }
}
