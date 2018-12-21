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
 * Context in log parser rule
 */
@Root(name="context", strict=false)
public class LogParserContext
{
	@Attribute(required=false)
	private String action = null;
	
	@Attribute(required=false)
	private String reset = null;
	
	@Text(required=false)
	private String data = ""; //$NON-NLS-1$

	/**
	 * @return the action
	 */
	public int getAction()
	{
		return (action == null) ? 0 : (action.equalsIgnoreCase("set") ? 0 : 1); //$NON-NLS-1$
	}

	/**
	 * @param action the action to set
	 */
	public void setAction(int action)
	{
		this.action = (action == 0) ? "set" : "clear"; //$NON-NLS-1$ //$NON-NLS-2$
	}

	/**
	 * @return the reset
	 */
	public int getReset()
	{
		return (reset == null) ? 0 : (reset.equalsIgnoreCase("auto") ? 0 : 1); //$NON-NLS-1$
	}

	/**
	 * @param reset the reset to set
	 */
	public void setReset(int reset)
	{
		this.reset = (action.equalsIgnoreCase("clear")) ? null : ((reset == 0) ? "auto" : "manual"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
	}

	/**
	 * @return the data
	 */
	public String getData()
	{
		return data;
	}

	/**
	 * @param data the data to set
	 */
	public void setData(String data)
	{
		this.data = data;
	}
}
