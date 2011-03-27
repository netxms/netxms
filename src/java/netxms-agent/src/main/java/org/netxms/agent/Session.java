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
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;

import org.netxms.agent.api.Parameter;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPException;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCPMessageReceiver;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Communication session
 */
public class Session
{
	private Socket socket;
	private ReceiverThread recvThread = null;
	private final Logger logger = LoggerFactory.getLogger(Session.class); 

	/**
	 * Receiver thread.
	 */
	private class ReceiverThread extends Thread
	{
		ReceiverThread()
		{
			setDaemon(true);
			setName("RecvThread:" + socket.getInetAddress().getHostAddress());
			start();
		}

		@Override
		public void run()
		{
			final NXCPMessageReceiver receiver = new NXCPMessageReceiver(4194304);
			InputStream in;

			try
			{
				in = socket.getInputStream();
			}
			catch(IOException e)
			{
				return; // Stop receiver thread if input stream cannot be obtained
			}

			while(socket.isConnected())
			{
				try
				{
					final NXCPMessage msg = receiver.receiveMessage(in);
					logger.debug("Message received: {}", msg.getMessageCode());
					switch(msg.getMessageCode())
					{
						case NXCPCodes.CMD_GET_NXCP_CAPS:
							processProtocolSetup(msg);
							break;
						case NXCPCodes.CMD_KEEPALIVE:
							processKeepaliveMessage(msg);
							break;
						case NXCPCodes.CMD_GET_PARAMETER:
							getParameter(msg);
							break;
						default:
							break;
					}
				}
				catch(IOException e)
				{
					break; // Stop on read errors
				}
				catch(NXCPException e)
				{
					if (e.getErrorCode() == NXCPCodes.ERR_CONNECTION_CLOSED)
						break;
				}
			}
			
			try
			{
				socket.close();
			}
			catch(IOException e)
			{
			}
			logger.info("session closed");
		}
	}
	
	/**
	 * Create new communication session
	 * @param socket
	 */
	public Session(Socket socket)
	{
		this.socket = socket;
		recvThread = new ReceiverThread();
		logger.info("New session created with {}", socket.getInetAddress().getHostAddress());
	}
	
	/**
	 * Disconnect session
	 */
	public void disconnect()
	{
		try
		{
			socket.shutdownInput();
			socket.shutdownOutput();
		}
		catch(IOException e)
		{
		}
	}
	
	/**
	 * Send message to server
	 * 
	 * @param msg
	 *           Message to sent
	 * @throws IOException
	 *            if case of socket communication failure
	 */
	private synchronized void sendMessage(final NXCPMessage msg) throws IOException
	{
		final OutputStream outputStream = socket.getOutputStream();
		final byte[] message = msg.createNXCPMessage();
		outputStream.write(message);
		logger.debug("Sent message {}", msg.getMessageCode());
	}
	
	/**
	 * Create response message
	 * 
	 * @param request request message
	 * @return response message
	 */
	private NXCPMessage createResponseMessage(NXCPMessage request)
	{
		return new NXCPMessage(NXCPCodes.CMD_REQUEST_COMPLETED, request.getMessageId());
	}
	
	/**
	 * Process protocol setup
	 * @param request request message
	 * @throws IOException 
	 */
	private void processProtocolSetup(NXCPMessage request) throws IOException
	{
		final NXCPMessage response = new NXCPMessage(NXCPCodes.CMD_NXCP_CAPS, request.getMessageId());
		response.setControl(true);
		response.setControlData(0x02000000);	// NXCP version 2
		sendMessage(response);
	}
	
	/**
	 * Process keepalive message
	 * @throws IOException 
	 */
	private void processKeepaliveMessage(NXCPMessage request) throws IOException
	{
		final NXCPMessage response = createResponseMessage(request);
		response.setVariableInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
		sendMessage(response);
	}

	/**
	 * Get requested parameter
	 * 
	 * @param msg NXCP message with request
	 * @throws IOException 
	 */
	private void getParameter(NXCPMessage request) throws IOException
	{
		final NXCPMessage response = createResponseMessage(request);

		final String parameter = request.getVariableAsString(NXCPCodes.VID_PARAMETER);
		logger.debug("requesting parameter \"{}\"", parameter);
		Parameter p = Agent.getInstance().findParameter(parameter);
		if (p != null)
		{
			final String result = p.handler(parameter);
			if (result == Parameter.RC_NOT_SUPPORTED)
			{
				response.setVariableInt32(NXCPCodes.VID_RCC, RCC.UNKNOWN_PARAMETER);
			}
			else if (result == Parameter.RC_ERROR)
			{
				response.setVariableInt32(NXCPCodes.VID_RCC, RCC.INTERNAL_ERROR);
			}
			else
			{
				response.setVariableInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
				response.setVariable(NXCPCodes.VID_VALUE, result);
			}
		}
		else
		{
			response.setVariableInt32(NXCPCodes.VID_RCC, RCC.UNKNOWN_PARAMETER);
		}

		sendMessage(response);
	}
}
