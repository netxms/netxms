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

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * DCI summary table
 */
public class DciSummaryTable
{
	private int id;
   private UUID guid;
	private String menuPath;
	private String title;
	private int flags;
	private String nodeFilter;
	private List<DciSummaryTableColumn> columns;
	
	/**
	 * Create new empty summary table object
	 */
	public DciSummaryTable(String menuPath, String title)
	{
		id = 0;
      guid = UUID.randomUUID();
		this.menuPath = menuPath;
		this.title = title;
		flags = 0;
		nodeFilter = "";
		columns = new ArrayList<DciSummaryTableColumn>();
	}
	
	/**
	 * Create full object from NXCP message.
	 * 
	 * @param msg
	 * @param baseId
	 */
	public DciSummaryTable(NXCPMessage msg)
	{
		id = msg.getFieldAsInt32(NXCPCodes.VID_SUMMARY_TABLE_ID);
		guid = msg.getFieldAsUUID(NXCPCodes.VID_GUID);
		menuPath = msg.getFieldAsString(NXCPCodes.VID_MENU_PATH);
		title = msg.getFieldAsString(NXCPCodes.VID_TITLE);
		flags = msg.getFieldAsInt32(NXCPCodes.VID_FLAGS);
		nodeFilter = msg.getFieldAsString(NXCPCodes.VID_FILTER);
		
		String s = msg.getFieldAsString(NXCPCodes.VID_COLUMNS);
		if ((s != null) && (s.length() > 0))
		{
			String[] parts = s.split("\\^\\~\\^");
			columns = new ArrayList<DciSummaryTableColumn>(parts.length);
			for(int i = 0; i < parts.length; i++)
			{
				String[] data = parts[i].split("\\^\\#\\^");
				if (data.length == 2)
				{
					columns.add(new DciSummaryTableColumn(data[0], data[1], 0));
				}
				else if (data.length == 3)
				{
					int flags;
					try
					{
						flags = Integer.parseInt(data[2]);
					}
					catch(NumberFormatException e)
					{
						flags = 0;
					}
					columns.add(new DciSummaryTableColumn(data[0], data[1], flags));
				}
			}
		}
		else
		{
			columns = new ArrayList<DciSummaryTableColumn>(0);
		}
	}
	
	/**
	 * Fill NXCP message with object data
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(NXCPMessage msg)
	{
		msg.setFieldInt32(NXCPCodes.VID_SUMMARY_TABLE_ID, id);
		msg.setField(NXCPCodes.VID_GUID, guid);
		msg.setField(NXCPCodes.VID_MENU_PATH, menuPath);
		msg.setField(NXCPCodes.VID_TITLE, title);
		msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
		msg.setField(NXCPCodes.VID_FILTER, nodeFilter);
		
		StringBuilder sb = new StringBuilder();
		for(DciSummaryTableColumn c : columns)
		{
			if (sb.length() > 0)
				sb.append("^~^");
			sb.append(c.getName());
			sb.append("^#^");
			sb.append(c.getDciName());
			sb.append("^#^");
			sb.append(c.getFlags());
		}
		msg.setField(NXCPCodes.VID_COLUMNS, sb.toString());
	}

	/**
	 * @return the id
	 */
	public int getId()
	{
		return id;
	}

   /**
	 * @param id the id to set
	 */
	public void setId(int id)
	{
		this.id = id;
	}

	/**
    * @return the guid
    */
   public UUID getGuid()
   {
      return guid;
   }

	/**
	 * @return the menuPath
	 */
	public String getMenuPath()
	{
		return menuPath;
	}

	/**
	 * @param menuPath the menuPath to set
	 */
	public void setMenuPath(String menuPath)
	{
		this.menuPath = menuPath;
	}

	/**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @param title the title to set
	 */
	public void setTitle(String title)
	{
		this.title = title;
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

	/**
	 * @return the nodeFilter
	 */
	public String getNodeFilter()
	{
		return nodeFilter;
	}

	/**
	 * @param nodeFilter the nodeFilter to set
	 */
	public void setNodeFilter(String nodeFilter)
	{
		this.nodeFilter = nodeFilter;
	}

	/**
	 * @return the columns
	 */
	public List<DciSummaryTableColumn> getColumns()
	{
		return columns;
	}
}
