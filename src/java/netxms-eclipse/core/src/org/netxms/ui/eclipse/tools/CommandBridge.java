/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.tools;

import java.util.HashMap;
import java.util.Map;
import org.netxms.ui.eclipse.console.Activator;

/**
 * Command bridge - allows one plugin to register action and other plugin to execute it by name
 */
public final class CommandBridge
{
	private static CommandBridge instance = new CommandBridge();
	
	/**
	 * Get instance
	 * 
	 * @return
	 */
	public static CommandBridge getInstance()
	{
		return instance;
	}
	
	private Map<String, Command> commands;
	
	/**
	 * Private constructor
	 */
	private CommandBridge()
	{
		commands = new HashMap<String, Command>();
	}
	
	/**
	 * @param name
	 * @param cmd
	 */
	public void registerCommand(String name, Command cmd)
	{
		commands.put(name, cmd);
	}
	
	/**
	 * @param name
	 */
	public void unregisterCommand(String name)
	{
		commands.remove(name);
	}
	
	/**
	 * @param name
	 * @param arg
	 * @return
	 */
	public Object execute(String name, Object arg)
	{
		try
		{
			Command c = commands.get(name);
			return (c != null) ? c.execute(name, arg) : null;
		}
		catch(Exception e)
		{
			Activator.logError("Exception while calling handler for bridged command " + name, e); //$NON-NLS-1$
			return null;
		}
	}
}
