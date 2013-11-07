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

import org.netxms.base.NXCPMessage;

/**
 * Read-only representation of NetXMS event. Intended to be created only
 * by client library communication module.
 *
 */
public class Event
{
	private final long id;
	private final int code;
	private final Date timeStamp;
	private final long sourceId;
	private final int severity;
	private final String message;
	private final String userTag;
	private final String[] parameters;
	
	/**
	 * Create event object from NXCP message. Intended to be called only by NXCSession.
	 * 
	 * @param msg NXCP message
	 */
	public Event(final NXCPMessage msg, final long baseId)
	{
		long varId = baseId;
		
		id = msg.getVariableAsInt64(varId++);
		code = msg.getVariableAsInteger(varId++);
		timeStamp = msg.getVariableAsDate(varId++);
		sourceId = msg.getVariableAsInt64(varId++);
		severity = msg.getVariableAsInteger(varId++);
		message = msg.getVariableAsString(varId++);
		userTag = msg.getVariableAsString(varId++);
		
		int count = msg.getVariableAsInteger(varId++);
		parameters = new String[count];
		for(int i = 0; i < count; i++)
		{
			parameters[i] = msg.getVariableAsString(varId++);
		}
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the code
	 */
	public int getCode()
	{
		return code;
	}

	/**
	 * @return the timeStamp
	 */
	public Date getTimeStamp()
	{
		return timeStamp;
	}

	/**
	 * @return the sourceId
	 */
	public long getSourceId()
	{
		return sourceId;
	}

	/**
	 * @return the severity
	 */
	public int getSeverity()
	{
		return severity;
	}

	/**
	 * @return the message
	 */
	public String getMessage()
	{
		return message;
	}

	/**
	 * @return the userTag
	 */
	public String getUserTag()
	{
		return userTag;
	}

	/**
	 * @return the parameters
	 */
	public String[] getParameters()
	{
		return parameters;
	}
}
