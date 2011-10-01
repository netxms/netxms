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
package org.netxms.client.objects;

import java.util.ArrayList;
import java.util.List;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;

/**
 * Dashboard object
 */
public class Dashboard extends GenericObject
{
	public static final int EQUAL_WIDTH_COLUMNS = 0x0001;
	
	private int numColumns;
	private int options;
	private List<DashboardElement> elements;

	/**
	 * @param msg
	 * @param session
	 */
	public Dashboard(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		numColumns = msg.getVariableAsInteger(NXCPCodes.VID_NUM_COLUMNS);
		options = msg.getVariableAsInteger(NXCPCodes.VID_FLAGS);
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_ELEMENTS);
		elements = new ArrayList<DashboardElement>(count);
		long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			elements.add(new DashboardElement(msg, varId));
			varId += 10;
		}
	}

	/**
	 * @return the numColumns
	 */
	public int getNumColumns()
	{
		return numColumns;
	}

	/**
	 * @return the elements
	 */
	public List<DashboardElement> getElements()
	{
		return elements;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Dashboard";
	}

	/**
	 * @return the options
	 */
	public int getOptions()
	{
		return options;
	}
}
