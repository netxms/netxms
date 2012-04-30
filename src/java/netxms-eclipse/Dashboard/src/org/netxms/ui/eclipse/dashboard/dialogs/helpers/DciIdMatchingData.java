/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.dialogs.helpers;

/**
 * Data for DCI ID matching
 */
public class DciIdMatchingData implements IdMatchingData
{
	public long srcNodeId;
	public long srcDciId;
	public String srcName;
	public long dstNodeId;
	public long dstDciId;
	public String dstName;
	
	/**
	 * @param srcNodeId
	 * @param srcDciId
	 * @param srcName
	 */
	public DciIdMatchingData(long srcNodeId, long srcDciId, String srcName)
	{
		this.srcNodeId = srcNodeId;
		this.srcDciId = srcDciId;
		this.srcName = srcName;
		this.dstNodeId = 0;
		this.dstDciId = 0;
		this.dstName = null;
	}

	@Override
	public long getSourceId()
	{
		return srcDciId;
	}

	@Override
	public String getSourceName()
	{
		return srcName;
	}

	@Override
	public long getDestinationId()
	{
		return dstDciId;
	}

	@Override
	public String getDestinationName()
	{
		return dstName;
	}
}
