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

import java.util.ArrayList;
import java.util.List;

/**
 * Data for object ID matching
 */
public class ObjectIdMatchingData implements IdMatchingData
{
	public long srcId;
	public String srcName;
	public int objectClass;
	public long dstId;
	public String dstName;
	public List<DciIdMatchingData> dcis = new ArrayList<DciIdMatchingData>();
	
	/**
	 * @param srcId
	 * @param srcName
	 * @param objectClass
	 */
	public ObjectIdMatchingData(long srcId, String srcName, int objectClass)
	{
		this.srcId = srcId;
		this.srcName = srcName;
		this.objectClass = objectClass;
		this.dstId = 0;
		this.dstName = null;
	}

	@Override
	public long getSourceId()
	{
		return srcId;
	}

	@Override
	public String getSourceName()
	{
		return srcName;
	}

	@Override
	public long getDestinationId()
	{
		return dstId;
	}

	@Override
	public String getDestinationName()
	{
		return dstName;
	}
}
