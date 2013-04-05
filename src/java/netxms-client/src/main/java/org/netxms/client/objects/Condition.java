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
package org.netxms.client.objects;

import java.util.ArrayList;
import java.util.List;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.ConditionDciInfo;

/**
 * Condition object
 *
 */
public class Condition extends GenericObject
{
	private String script;
	private int activationEvent;
	private int deactivationEvent;
	private long eventSourceObject;
	private int activeStatus;
	private int inactiveStatus;
	private List<ConditionDciInfo> dciList;
	
	/**
	 * @param msg
	 * @param session
	 */
	public Condition(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		script = msg.getVariableAsString(NXCPCodes.VID_SCRIPT);
		activationEvent = msg.getVariableAsInteger(NXCPCodes.VID_ACTIVATION_EVENT);
		deactivationEvent = msg.getVariableAsInteger(NXCPCodes.VID_DEACTIVATION_EVENT);
		eventSourceObject = msg.getVariableAsInt64(NXCPCodes.VID_SOURCE_OBJECT);
		activeStatus = msg.getVariableAsInteger(NXCPCodes.VID_ACTIVE_STATUS);
		inactiveStatus = msg.getVariableAsInteger(NXCPCodes.VID_INACTIVE_STATUS);
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_ITEMS);
		dciList = new ArrayList<ConditionDciInfo>(count);
		
		long varId = NXCPCodes.VID_DCI_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			dciList.add(new ConditionDciInfo(msg, varId));
			varId += 10;
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.NXCObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Condition";
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
	 */
	@Override
	public boolean isAllowedOnMap()
	{
		return true;
	}

	/**
	 * @return the script
	 */
	public String getScript()
	{
		return script;
	}

	/**
	 * @return the activationEvent
	 */
	public int getActivationEvent()
	{
		return activationEvent;
	}

	/**
	 * @return the deactivationEvent
	 */
	public int getDeactivationEvent()
	{
		return deactivationEvent;
	}

	/**
	 * @return the eventSourceObject
	 */
	public long getEventSourceObject()
	{
		return eventSourceObject;
	}

	/**
	 * @return the activeStatus
	 */
	public int getActiveStatus()
	{
		return activeStatus;
	}

	/**
	 * @return the inactiveStatus
	 */
	public int getInactiveStatus()
	{
		return inactiveStatus;
	}

	/**
	 * @return the dciList
	 */
	public List<ConditionDciInfo> getDciList()
	{
		return dciList;
	}
}
