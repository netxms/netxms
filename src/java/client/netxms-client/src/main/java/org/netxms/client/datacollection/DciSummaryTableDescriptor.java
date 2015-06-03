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

import java.util.UUID;
import org.netxms.base.NXCPMessage;

/**
 * Descriptor of DCI summary table
 */
public class DciSummaryTableDescriptor
{
	private int id;
   private UUID guid;
	private String menuPath;
	private String title;
	private int flags;
	
	/**
	 * Create summary information from NXCP message.
	 * 
	 * @param msg
	 * @param baseId
	 */
	public DciSummaryTableDescriptor(NXCPMessage msg, long baseId)
	{
		id = msg.getFieldAsInt32(baseId);
		menuPath = msg.getFieldAsString(baseId + 1);
		title = msg.getFieldAsString(baseId + 2);
		flags = msg.getFieldAsInt32(baseId + 3);
		guid = msg.getFieldAsUUID(baseId + 4);
	}

	/**
	 * Create descriptor from table object
	 * 
	 * @param table
	 */
	public DciSummaryTableDescriptor(DciSummaryTable table)
	{
		id = table.getId();
		menuPath = table.getMenuPath();
		title = table.getTitle();
		flags = table.getFlags();
		guid = table.getGuid();
	}
	
	/**
	 * Update descriptor from table
	 * 
	 * @param table
	 */
	public void updateFromTable(DciSummaryTable table)
	{
		menuPath = table.getMenuPath();
		title = table.getTitle();
		flags = table.getFlags();
	}
	
	/**
	 * @return the id
	 */
	public int getId()
	{
		return id;
	}

	/**
	 * @return the menuPath
	 */
	public String getMenuPath()
	{
		return menuPath;
	}

	/**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

   /**
    * @return the guid
    */
   public UUID getGuid()
   {
      return guid;
   }
}
