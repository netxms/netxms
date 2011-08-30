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

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.Threshold;

/**
 * Business service check
 */
public class ServiceCheck extends GenericObject
{
	public static final int CHECK_TYPE_SCRIPT = 1;
	public static final int CHECK_TYPE_THRESHOLD = 2;
	
	private int checkType;
	private String script;
	private Threshold threshold;
	private String failureReason;

	/**
	 * @param msg
	 * @param session
	 */
	public ServiceCheck(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		checkType = msg.getVariableAsInteger(NXCPCodes.VID_SLMCHECK_TYPE);
		script = msg.getVariableAsString(NXCPCodes.VID_SCRIPT); 
		threshold = new Threshold(msg, NXCPCodes.VID_THRESHOLD_BASE);
		failureReason = msg.getVariableAsString(NXCPCodes.VID_REASON);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "ServiceCheck";
	}

	/**
	 * @return the checkType
	 */
	public int getCheckType()
	{
		return checkType;
	}

	/**
	 * @return the script
	 */
	public String getScript()
	{
		return script;
	}

	/**
	 * @return the threshold
	 */
	public Threshold getThreshold()
	{
		return threshold;
	}

	/**
	 * @return the failureReason
	 */
	public String getFailureReason()
	{
		return failureReason;
	}
}
