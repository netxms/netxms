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
package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Text;

/**
 * Event in log parser rule
 */
@Root(name="event", strict=false)
public class LogParserEvent
{
	@Text
	private String event = "0"; //$NON-NLS-1$

	@Attribute(name="params", required=false)
	private Integer parameterCount = null;
	
	/**
	 * Protected constructor for XML parser
	 */
	protected LogParserEvent()
	{
	}
	
	/**
	 * @param event
	 * @param parameterCount
	 */
	public LogParserEvent(String event, Integer parameterCount)
	{
		this.event = event;
		this.parameterCount = parameterCount;
	}

	/**
	 * @return the parameterCount
	 */
	public int getParameterCount()
	{
		return parameterCount;
	}

	/**
	 * @param parameterCount the parameterCount to set
	 */
	public void setParameterCount(Integer parameterCount)
	{
		this.parameterCount = parameterCount;
	}

	/**
	 * @return the event
	 */
	public String getEvent()
	{
		return event;
	}

	/**
	 * @param event the event to set
	 */
	public void setEvent(String event)
	{
		this.event = event;
	}
}
