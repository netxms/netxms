/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.agent;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;

import org.netxms.agent.api.Parameter;
import org.netxms.base.Glob;
import org.netxms.base.NXCommon;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Man class for NetXMS Java Agent
 *
 */
public class Agent
{
	// Agent's instance
	private static Agent instance = null;
	
	// Environment
	private final Logger logger = LoggerFactory.getLogger(Agent.class);
	
	// Communication data
	private int listenPort = 4700;
	private ServerSocket socket;
	
	// Supported parameters
	List<Parameter> parameters = new ArrayList<Parameter>();
	
	/**
	 * Create agent's instance
	 */
	private Agent()
	{
		parameters.add(new Parameter("Agent.Version", "Agent version", Parameter.DT_STRING) {
			@Override
			public String handler(String parameter)
			{
				return NXCommon.VERSION;
			}
		});
		parameters.add(new Parameter("System.PlatformName", "Paltform name", Parameter.DT_STRING) {
			@Override
			public String handler(String parameter)
			{
				return System.getProperty("os.name") + "-" + System.getProperty("os.arch");
			}
		});
	}
	
	/**
	 * Start agent
	 * @throws IOException 
	 */
	private void start() throws IOException
	{
		socket = new ServerSocket(listenPort);
		logger.info("Listening on port {}", listenPort);
		while(true)
		{
			try
			{
				Socket client = socket.accept();
				Session session = new Session(client);
			}
			catch(IOException e)
			{
				break;
			}
		}
		socket.close();
	}
	
	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
		try
		{
			instance = new Agent();
			instance.start();
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
	}

	/**
	 * @return the instance
	 */
	public static Agent getInstance()
	{
		return instance;
	}
	
	/**
	 * Find parameter object by actual name passed to agent
	 * 
	 * @param name actual parameter's name
	 * @return parameter object or null if not found
	 */
	public synchronized Parameter findParameter(String name)
	{
		for(Parameter p : parameters)
			if (Glob.matchIgnoreCase(p.getName(), name))
				return p;
		return null;
	}
}
