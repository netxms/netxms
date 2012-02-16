/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.client.log;

import org.netxms.base.NXCPMessage;

/**
 * @author Victor
 *
 */
public class LogColumn
{
	// Column types
	public static final int LC_TEXT           = 0;
	public static final int LC_SEVERITY       = 1;
	public static final int LC_OBJECT_ID      = 2;
	public static final int LC_USER_ID        = 3;
	public static final int LC_EVENT_CODE     = 4;
	public static final int LC_TIMESTAMP      = 5;
	public static final int LC_INTEGER        = 6;
	public static final int LC_ALARM_STATE    = 7;
	public static final int LC_ALARM_HD_STATE = 8;

	private String name;
	private String description;
	private int type;
	
	/**
	 * Create log column object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId Base variable ID
	 */
	protected LogColumn(NXCPMessage msg, long baseId)
	{
		name = msg.getVariableAsString(baseId);
		type = msg.getVariableAsInteger(baseId + 1);
		description = msg.getVariableAsString(baseId + 2);
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}
}
