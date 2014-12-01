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
package org.netxms.client.datacollection;

import org.netxms.base.NXCPMessage;

/**
 * Column definition for DCI summary table
 */
public class DciSummaryTableColumn
{
	public static int REGEXP_MATCH = 0x0001;
	
	private String name;
	private String dciName;
	private int flags;

	/**
	 * @param name
	 * @param dciName
	 */
	public DciSummaryTableColumn(String name, String dciName, int flags)
	{
		this.name = name;
		this.dciName = dciName;
		this.flags = flags;
	}
	
	/**
	 * Copy constructor.
	 * 
	 * @param src
	 */
	public DciSummaryTableColumn(DciSummaryTableColumn src)
	{
		name = src.name;
		dciName = src.dciName;
		flags = src.flags;
	}
	
	/**
	 * @param msg
	 * @param baseId
	 */
	public void fillMessage(NXCPMessage msg, long baseId)
	{
	   msg.setField(baseId, name);
      msg.setField(baseId + 1, dciName);
      msg.setFieldInt32(baseId + 2, flags);
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the dciName
	 */
	public String getDciName()
	{
		return dciName;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @param dciName the dciName to set
	 */
	public void setDciName(String dciName)
	{
		this.dciName = dciName;
	}
	
	/**
	 * @return
	 */
	public boolean isRegexpMatch()
	{
		return (flags & REGEXP_MATCH) != 0;
	}
	
	/**
	 * @param enable
	 */
	public void setRegexpMatch(boolean enable)
	{
		if (enable)
			flags |= REGEXP_MATCH;
		else
			flags &= ~REGEXP_MATCH;
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

	@Override
	public String toString() {
		return "DciSummaryTableColumn{" +
				"name='" + name + '\'' +
				", dciName='" + dciName + '\'' +
				", flags=" + flags +
				'}';
	}
}
