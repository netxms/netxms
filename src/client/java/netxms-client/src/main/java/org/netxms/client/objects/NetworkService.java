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

import org.netxms.base.InetAddressEx;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.interfaces.NodeComponent;

/**
 * Network Service object
 */
public class NetworkService extends GenericObject implements NodeComponent
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
	InetAddressEx ipAddress;
	int protocol;
	int port;
	String request;
	String response;
	long pollerNode;
	int pollCount;
	
	/**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
	 */
	public NetworkService(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		serviceType = msg.getFieldAsInt32(NXCPCodes.VID_SERVICE_TYPE);
		ipAddress = msg.getFieldAsInetAddressEx(NXCPCodes.VID_IP_ADDRESS);
		protocol = msg.getFieldAsInt32(NXCPCodes.VID_IP_PROTO);
		port = msg.getFieldAsInt32(NXCPCodes.VID_IP_PORT);
		request = msg.getFieldAsString(NXCPCodes.VID_SERVICE_REQUEST);
		response = msg.getFieldAsString(NXCPCodes.VID_SERVICE_RESPONSE);
		pollerNode = msg.getFieldAsInt64(NXCPCodes.VID_POLLER_NODE_ID);
		pollCount = msg.getFieldAsInt32(NXCPCodes.VID_REQUIRED_POLLS);
	}

	/**
	 * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
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
    * @return the ipAddress
    */
   public InetAddressEx getIpAddress()
   {
      return ipAddress;
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

   /**
    * @see org.netxms.client.objects.interfaces.NodeComponent#getParentNode()
    */
   @Override
   public AbstractNode getParentNode()
   {
      AbstractNode node = null;
      synchronized(parents)
      {
         for(Long id : parents)
         {
            AbstractObject object = session.findObjectById(id);
            if (object instanceof AbstractNode)
            {
               node = (AbstractNode)object;
               break;
            }
         }
      }
      return node;
   }
}
