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
package org.netxms.client.datacollection;

import org.netxms.base.NXCPMessage;

/**
 * DCI information for performance tab in console
 */
public class PerfTabDci
{
	private long id;		// DCI ID
	private long templateDciId;	// ID of related template DCI, or 0 for standalone DCIs
	private int status;
	private String description;
	private String perfTabSettings;
	private String instance;
	
	public PerfTabDci(NXCPMessage msg, long baseId)
	{
		id = msg.getVariableAsInt64(baseId);
		description = msg.getVariableAsString(baseId + 1);
		status = msg.getVariableAsInteger(baseId + 2);
		perfTabSettings = msg.getVariableAsString(baseId + 3);
		templateDciId = msg.getVariableAsInt64(baseId + 5);
		instance = msg.getVariableAsString(baseId + 6);
		if (instance == null)
			instance = "";
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the status
	 */
	public int getStatus()
	{
		return status;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the perfTabSettings
	 */
	public String getPerfTabSettings()
	{
		return perfTabSettings;
	}

	/**
	 * @return the templateDciId
	 */
	public final long getTemplateDciId()
	{
		return templateDciId;
	}

	/**
	 * @return the instance
	 */
	public final String getInstance()
	{
		return instance;
	}
}
