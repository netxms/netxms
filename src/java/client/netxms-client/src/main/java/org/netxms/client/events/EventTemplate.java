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

import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.Severity;

/**
 * Event template
 */
public class EventTemplate extends EventObject
{
	private Severity severity;
	private int flags;
	private String message;
	
	/**
	 * Create new empty event template.
	 * 
	 * @param code Event code assigned by server
	 */
	public EventTemplate(long code)
	{
	   super(code);
		severity = Severity.NORMAL;
		flags = FLAG_WRITE_TO_LOG;
		message = "";
	}
	
	/**
	 * Create event template object from NXCP message.
	 * 
	 * @param msg NXCP message
    * @param base base field id
	 */
	public EventTemplate(final NXCPMessage msg, long base)
	{
	   super(msg, base);
		severity = Severity.getByValue(msg.getFieldAsInt32(base + 4));
		flags = msg.getFieldAsInt32(base + 5);
		message = msg.getFieldAsString(base + 6);
	}
	
	/**
	 * Copy constructor.
	 * 
	 * @param src Original event template object
	 */
	public EventTemplate(final EventTemplate src)
	{
	   super(src);
		setAll(src);
	}
	
	/**
	 * Set all attributes from another event template object.
	 * 
	 * @param src Original event template object
	 */
	public void setAll(final EventTemplate src)
	{
		severity = src.severity;
		flags = src.flags;
		message = src.message;
	}

	/**
	 * @return the severity
	 */
	public Severity getSeverity()
	{
		return severity;
	}

	/**
	 * @param severity the severity to set
	 */
	public void setSeverity(Severity severity)
	{
		this.severity = severity;
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

	/**
	 * @param flags the flags to set
	 */
	public void setFlags(int flags)
	{
		this.flags = flags;
	}

	/**
	 * @return the message
	 */
	public String getMessage()
	{
		return message;
	}

	/**
	 * @param message the message to set
	 */
	public void setMessage(String message)
	{
		this.message = message;
	}
}
