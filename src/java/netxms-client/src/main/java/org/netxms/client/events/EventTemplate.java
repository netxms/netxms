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

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.Severity;

/**
 * Event template
 */
public class EventTemplate
{
	public static final int FLAG_WRITE_TO_LOG = 0x0001;
	
	private long code;
	private String name;
	private int severity;
	private int flags;
	private String message;
	private String description;
	
	/**
	 * Create new empty event template.
	 * 
	 * @param code Event code assigned by server
	 */
	public EventTemplate(long code)
	{
		this.code = code;
		name = "";
		severity = Severity.NORMAL;
		flags = FLAG_WRITE_TO_LOG;
		message = "";
		description = "";
	}
	
	/**
	 * Create event template object from NXCP message.
	 * 
	 * @param msg NXCP message
	 */
	public EventTemplate(final NXCPMessage msg)
	{
		code = msg.getVariableAsInt64(NXCPCodes.VID_EVENT_CODE);
		severity = msg.getVariableAsInteger(NXCPCodes.VID_SEVERITY);
		flags = msg.getVariableAsInteger(NXCPCodes.VID_FLAGS);
		name = msg.getVariableAsString(NXCPCodes.VID_NAME);
		message = msg.getVariableAsString(NXCPCodes.VID_MESSAGE);
		description = msg.getVariableAsString(NXCPCodes.VID_DESCRIPTION);
	}
	
	/**
	 * Copy constructor.
	 * 
	 * @param src Original event template object
	 */
	public EventTemplate(final EventTemplate src)
	{
		setAll(src);
	}
	
	/**
	 * Set all attributes from another event template object.
	 * 
	 * @param src Original event template object
	 */
	public void setAll(final EventTemplate src)
	{
		code = src.code;
		severity = src.severity;
		flags = src.flags;
		name = src.name;
		message = src.message;
		description = src.description;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return the severity
	 */
	public int getSeverity()
	{
		return severity;
	}

	/**
	 * @param severity the severity to set
	 */
	public void setSeverity(int severity)
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

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @param description the description to set
	 */
	public void setDescription(String description)
	{
		this.description = description;
	}

	/**
	 * @return the code
	 */
	public long getCode()
	{
		return code;
	}

	/**
	 * @param code the code to set
	 */
	public void setCode(long code)
	{
		this.code = code;
	}
}
