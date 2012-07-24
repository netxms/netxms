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
package org.netxms.client.events;

import java.util.Date;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;


/**
 * Alarm
 */
public class Alarm
{
	// Alarm states
	public static final int STATE_OUTSTANDING = 0;
	public static final int STATE_ACKNOWLEDGED = 1;
	public static final int STATE_RESOLVED = 2;
	public static final int STATE_TERMINATED = 3;
	
	public static final int STATE_MASK = 0x0F;
	
	// Alarm helpdesk states
	public static final int HELPDESK_STATE_IGNORED = 0;
	public static final int HELPDESK_STATE_OPEN = 1;
	public static final int HELPDESK_STATE_CLOSED = 2;
	
	// Alarm attributes
	private long id;
	private int currentSeverity;
	private int originalSeverity;
	private int repeatCount;
	private int state;
	private boolean sticky;
	private int ackByUser;
	private int resolvedByUser;
	private int terminateByUser;
	private long sourceEventId;
	private int sourceEventCode;
	private long sourceObjectId;
	private Date creationTime;
	private Date lastChangeTime;
	private String message;
	private String key;
	private int helpdeskState;
	private String helpdeskReference;
	private int timeout;
	private int timeoutEvent;
	private int commentsCount;
	
	/**
	 * @param msg Source NXCP message
	 */
	public Alarm(NXCPMessage msg)
	{
		id = msg.getVariableAsInt64(NXCPCodes.VID_ALARM_ID);
		currentSeverity = msg.getVariableAsInteger(NXCPCodes.VID_CURRENT_SEVERITY);
		originalSeverity = msg.getVariableAsInteger(NXCPCodes.VID_ORIGINAL_SEVERITY);
		repeatCount = msg.getVariableAsInteger(NXCPCodes.VID_REPEAT_COUNT);
		state = msg.getVariableAsInteger(NXCPCodes.VID_STATE);
		sticky = msg.getVariableAsBoolean(NXCPCodes.VID_IS_STICKY);
		ackByUser = msg.getVariableAsInteger(NXCPCodes.VID_ACK_BY_USER);
		resolvedByUser = msg.getVariableAsInteger(NXCPCodes.VID_RESOLVED_BY_USER);
		terminateByUser = msg.getVariableAsInteger(NXCPCodes.VID_TERMINATED_BY_USER);
		sourceEventId = msg.getVariableAsInt64(NXCPCodes.VID_EVENT_ID);
		sourceEventCode = msg.getVariableAsInteger(NXCPCodes.VID_EVENT_CODE);
		sourceObjectId = msg.getVariableAsInt64(NXCPCodes.VID_OBJECT_ID);
		creationTime = new Date(msg.getVariableAsInt64(NXCPCodes.VID_CREATION_TIME) * 1000);
		lastChangeTime = new Date(msg.getVariableAsInt64(NXCPCodes.VID_LAST_CHANGE_TIME) * 1000);
		message = msg.getVariableAsString(NXCPCodes.VID_ALARM_MESSAGE);
		key = msg.getVariableAsString(NXCPCodes.VID_ALARM_KEY);
		helpdeskState = msg.getVariableAsInteger(NXCPCodes.VID_HELPDESK_STATE);
		helpdeskReference = msg.getVariableAsString(NXCPCodes.VID_HELPDESK_REF);
		timeout = msg.getVariableAsInteger(NXCPCodes.VID_ALARM_TIMEOUT);
		timeoutEvent = msg.getVariableAsInteger(NXCPCodes.VID_ALARM_TIMEOUT_EVENT);
		commentsCount = msg.getVariableAsInteger(NXCPCodes.VID_NUM_COMMENTS);
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the currentSeverity
	 */
	public int getCurrentSeverity()
	{
		return currentSeverity;
	}

	/**
	 * @return the originalSeverity
	 */
	public int getOriginalSeverity()
	{
		return originalSeverity;
	}

	/**
	 * @return the repeatCount
	 */
	public int getRepeatCount()
	{
		return repeatCount;
	}

	/**
	 * @return the state
	 */
	public int getState()
	{
		return state;
	}

	/**
	 * @return the ackByUser
	 */
	public int getAckByUser()
	{
		return ackByUser;
	}

	/**
	 * @return the terminateByUser
	 */
	public int getTerminateByUser()
	{
		return terminateByUser;
	}

	/**
	 * @return the sourceEventId
	 */
	public long getSourceEventId()
	{
		return sourceEventId;
	}

	/**
	 * @return the sourceEventCode
	 */
	public int getSourceEventCode()
	{
		return sourceEventCode;
	}

	/**
	 * @return the sourceObjectId
	 */
	public long getSourceObjectId()
	{
		return sourceObjectId;
	}

	/**
	 * @return the creationTime
	 */
	public Date getCreationTime()
	{
		return creationTime;
	}

	/**
	 * @return the lastChangeTime
	 */
	public Date getLastChangeTime()
	{
		return lastChangeTime;
	}

	/**
	 * @return the message
	 */
	public String getMessage()
	{
		return message;
	}

	/**
	 * @return the key
	 */
	public String getKey()
	{
		return key;
	}

	/**
	 * @return the helpdeskState
	 */
	public int getHelpdeskState()
	{
		return helpdeskState;
	}

	/**
	 * @return the helpdeskReference
	 */
	public String getHelpdeskReference()
	{
		return helpdeskReference;
	}

	/**
	 * @return the timeout
	 */
	public int getTimeout()
	{
		return timeout;
	}

	/**
	 * @return the timeoutEvent
	 */
	public int getTimeoutEvent()
	{
		return timeoutEvent;
	}

	/**
	 * @return the commentsCount
	 */
	public int getCommentsCount()
	{
		return commentsCount;
	}

	/**
	 * @return the resolvedByUser
	 */
	public int getResolvedByUser()
	{
		return resolvedByUser;
	}

	/**
	 * @return the sticky
	 */
	public boolean isSticky()
	{
		return sticky;
	}
}
