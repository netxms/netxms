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
package org.netxms.ui.eclipse.shared;

import org.eclipse.rap.rwt.RWT;
import org.netxms.api.client.Session;

/**
 * Compatibility class for RCP plugins
 */
public class ConsoleSharedData
{
	/**
	 * Get NetXMS session
	 * 
	 * @return
	 */
	public static Session getSession()
	{
		return (Session)RWT.getUISession().getAttribute("netxms.session");
	}

	/**
	 * Get value of console property
	 * 
	 * @param name name of the property
	 * @return property value or null
	 */
	public static Object getProperty(final String name)
	{
		return RWT.getUISession().getAttribute("netxms." + name);
	}
	
	/**
	 * Set value of console property
	 * 
	 * @param name name of the property
	 * @param value new property value
	 */
	public static void setProperty(final String name, final Object value)
	{
		RWT.getUISession().setAttribute("netxms." + name, value);
	}
}
