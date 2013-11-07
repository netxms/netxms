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
package org.netxms.client.objects;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Network Service object
 */
public class NetworkService extends GenericObject
{
	public static final int CUSTOM = 0;
	public static final int SSH = 1;
	public static final int POP3 = 2;
	public static final int SMTP = 3;
	public static final int FTP = 4;
	public static final int HTTP = 5;
	public static final int HTTPS = 6;
	public static final int TELNET = 7;
	
	int serviceType;
	int protocol;
	int port;
	String request;
	String response;
	long pollerNode;
	int pollCount;
	
	/**
	 * @param msg
	 * @param session
	 */
	public NetworkService(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		serviceType = msg.getVariableAsInteger(NXCPCodes.VID_SERVICE_TYPE);
		protocol = msg.getVariableAsInteger(NXCPCodes.VID_IP_PROTO);
		port = msg.getVariableAsInteger(NXCPCodes.VID_IP_PORT);
		request = msg.getVariableAsString(NXCPCodes.VID_SERVICE_REQUEST);
		response = msg.getVariableAsString(NXCPCodes.VID_SERVICE_RESPONSE);
		pollerNode = msg.getVariableAsInt64(NXCPCodes.VID_POLLER_NODE_ID);
		pollCount = msg.getVariableAsInteger(NXCPCodes.VID_REQUIRED_POLLS);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "NetworkService";
	}

	/**
	 * @return the serviceType
	 */
	public int getServiceType()
	{
		return serviceType;
	}

	/**
	 * @return the protocol
	 */
	public int getProtocol()
	{
		return protocol;
	}

	/**
	 * @return the port
	 */
	public int getPort()
	{
		return port;
	}

	/**
	 * @return the request
	 */
	public String getRequest()
	{
		return request;
	}

	/**
	 * @return the response
	 */
	public String getResponse()
	{
		return response;
	}

	/**
	 * @return the pollerNode
	 */
	public long getPollerNode()
	{
		return pollerNode;
	}

	/**
	 * @return the pollCount
	 */
	public int getPollCount()
	{
		return pollCount;
	}
}
