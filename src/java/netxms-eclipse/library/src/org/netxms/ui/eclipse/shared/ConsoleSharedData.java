/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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

import java.util.HashMap;
import java.util.Map;
import org.eclipse.swt.widgets.TrayItem;
import org.netxms.api.client.Session;

/**
 * Shared data for NXMC extensions
 * 
 */
public class ConsoleSharedData
{
	private static Session session = null;
	private static TrayItem trayIcon = null;
	private static Map<String, Object> consoleProperties = new HashMap<String, Object>(0);
	
	/**
	 * Get current NetXMS client library session
	 * 
	 * @return Current session
	 */
	public static Session getSession()
	{
		return session;
	}

	/**
	 * Set current NetXMS client library session
	 * 
	 * @param session Current session
	 */
	public static void setSession(Session session)
	{
		ConsoleSharedData.session = session;
	}
	
	/**
	 * Get value of console property
	 * 
	 * @param name name of the property
	 * @return property value or null
	 */
	public static Object getProperty(final String name)
	{
		return consoleProperties.get(name);
	}
	
	/**
	 * Set value of console property
	 * 
	 * @param name name of the property
	 * @param value new property value
	 */
	public static void setProperty(final String name, final Object value)
	{
		consoleProperties.put(name, value);
	}

	/**
	 * @return the trayIcon
	 */
	public static TrayItem getTrayIcon()
	{
		return trayIcon;
	}

	/**
	 * @param trayIcon the trayIcon to set
	 */
	public static void setTrayIcon(TrayItem trayIcon)
	{
		ConsoleSharedData.trayIcon = trayIcon;
	}
}
