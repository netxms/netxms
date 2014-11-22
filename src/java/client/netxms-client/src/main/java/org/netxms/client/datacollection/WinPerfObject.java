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
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Windows Performance Object 
 */
public class WinPerfObject
{
	private String name;
	private List<WinPerfCounter> counters;
	private List<String> instances;
	
	/**
	 * Create WinPerf objects list from NXCP message
	 * 
	 * @param msg
	 * @return
	 */
	public static List<WinPerfObject> createListFromMessage(NXCPMessage msg)
	{
		int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_OBJECTS);
		List<WinPerfObject> objects = new ArrayList<WinPerfObject>(count);
		
		long varId = NXCPCodes.VID_PARAM_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			WinPerfObject o = new WinPerfObject(msg, varId);
			varId += o.counters.size() + o.instances.size() + 3;
			objects.add(o);
		}
		return objects;
	}
	
	/**
	 * Create WinPerf object from NXCP message
	 */
	private WinPerfObject(NXCPMessage msg, long baseId)
	{
		name = msg.getFieldAsString(baseId);
		
		int count = msg.getFieldAsInt32(baseId + 1);
		counters = new ArrayList<WinPerfCounter>(count);
		long varId = baseId + 3;
		for(int i = 0; i < count; i++)
			counters.add(new WinPerfCounter(this, msg.getFieldAsString(varId++)));
		
		count = msg.getFieldAsInt32(baseId + 2);
		instances = new ArrayList<String>(count);
		for(int i = 0; i < count; i++)
			instances.add(msg.getFieldAsString(varId++));
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the counters
	 */
	public WinPerfCounter[] getCounters()
	{
		return counters.toArray(new WinPerfCounter[counters.size()]);
	}

	/**
	 * @return the instances
	 */
	public String[] getInstances()
	{
		return instances.toArray(new String[instances.size()]);
	}
	
	/**
	 * Returns true if this object has counters
	 * 
	 * @return
	 */
	public boolean hasCounters()
	{
		return counters.size() > 0;
	}
	
	/**
	 * Returns true if this object has instances
	 * 
	 * @return
	 */
	public boolean hasInstances()
	{
		return instances.size() > 0;
	}
}
