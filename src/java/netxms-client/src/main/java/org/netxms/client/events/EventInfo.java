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
package org.netxms.client.events;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import org.netxms.base.NXCPMessage;

/**
 * Information about NetXMS event
 */
public class EventInfo
{
	private long id;
	private Date timeStamp;
	private long sourceObjectId;
	private int code;
	private String name;
	private int severity;
	private String message;
	private EventInfo parent;
	private List<EventInfo> children;
	
	/**
	 * Create from NXCP message.
	 * 
	 * @param msg
	 * @param baseId
	 */
	public EventInfo(NXCPMessage msg, long baseId, EventInfo parent)
	{
		this.parent = parent;
		if (parent != null)
			parent.addChild(this);
		children = null;
		id = msg.getVariableAsInt64(baseId);
		code = msg.getVariableAsInteger(baseId + 2);
		name = msg.getVariableAsString(baseId + 3);
		severity = msg.getVariableAsInteger(baseId + 4);
		sourceObjectId = msg.getVariableAsInt64(baseId + 5);
		timeStamp = msg.getVariableAsDate(baseId + 6);
		message = msg.getVariableAsString(baseId + 7);
	}
	
	/**
	 * Add child event
	 * 
	 * @param e
	 */
	private void addChild(EventInfo e)
	{
		if (children == null)
			children = new ArrayList<EventInfo>();
		children.add(e);
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return parent event or null
	 */
	public EventInfo getParent()
	{
		return parent;
	}

	/**
	 * @return the timeStamp
	 */
	public Date getTimeStamp()
	{
		return timeStamp;
	}

	/**
	 * @return the sourceObjectId
	 */
	public long getSourceObjectId()
	{
		return sourceObjectId;
	}

	/**
	 * @return the code
	 */
	public int getCode()
	{
		return code;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
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
	 * @return the children
	 */
	public EventInfo[] getChildren()
	{
		return (children != null) ? children.toArray(new EventInfo[children.size()]) : new EventInfo[0];
	}
	
	/**
	 * @return
	 */
	public boolean hasChildren()
	{
		return (children != null) && (children.size() > 0);
	}
}
