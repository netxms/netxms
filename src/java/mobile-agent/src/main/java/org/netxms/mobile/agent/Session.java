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
package org.netxms.mobile.agent;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.security.GeneralSecurityException;
import java.util.concurrent.atomic.AtomicLong;
import org.netxms.base.EncryptionContext;
import org.netxms.base.GeoLocation;
import org.netxms.base.Logger;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPException;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCPMessageReceiver;
import org.netxms.base.NXCPMsgWaitQueue;
import org.netxms.base.NXCommon;
import org.netxms.mobile.agent.constants.RCC;

/**
 * Communication session with NetXMS server.
 */
public class Session
{
	// Various public constants
	public static final int DEFAULT_CONN_PORT = 4747;
	public static final int PROTOCOL_VERSION = 1;

	// Connection-related attributes
	private String connAddress;
	private int connPort;
	private String connDeviceId;
	private String connLoginName;
	private String connPassword;
	private boolean connUseEncryption;

	// Internal communication data
	private Socket connSocket = null;
	private NXCPMsgWaitQueue msgWaitQueue = null;
	private ReceiverThread recvThread = null;
	private AtomicLong requestId = new AtomicLong(1);
	private boolean isConnected = false;
	private EncryptionContext encryptionContext = null;

	// Communication parameters
	private int recvBufferSize = 65536; // Default is 64KB
	private int commandTimeout = 30000; // Default is 30 sec
	
	/**
	 * Setup encryption
	 * 
	 * @param msg CMD_REQUEST_SESSION_KEY message
	 * @throws IOException 
	 * @throws MobileAgentException 
	 */
	private void setupEncryption(NXCPMessage msg) throws IOException, MobileAgentException
	{
		final NXCPMessage response = new NXCPMessage(NXCPCodes.CMD_SESSION_KEY, msg.getMessageId());
		response.setEncryptionDisabled(true);

		try
		{
			encryptionContext = EncryptionContext.createInstance(msg);
			response.setVariable(NXCPCodes.VID_SESSION_KEY, encryptionContext.getEncryptedSessionKey(msg));
			response.setVariable(NXCPCodes.VID_SESSION_IV, encryptionContext.getEncryptedIv(msg));
			response.setVariableInt16(NXCPCodes.VID_CIPHER, encryptionContext.getCipher());
			response.setVariableInt16(NXCPCodes.VID_KEY_LENGTH, encryptionContext.getKeyLength());
			response.setVariableInt16(NXCPCodes.VID_IV_LENGTH, encryptionContext.getIvLength());
			response.setVariableInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
		}
		catch(Exception e)
		{
			response.setVariableInt32(NXCPCodes.VID_RCC, RCC.NO_CIPHERS);
		}
		
		sendMessage(response);
	}
	
	/**
	 * Receiver thread for NXCSession
	 */
	private class ReceiverThread extends Thread
	{
		ReceiverThread()
		{
			setDaemon(true);
			start();
		}

		@Override
		public void run()
		{
			final NXCPMessageReceiver receiver = new NXCPMessageReceiver(recvBufferSize);
			InputStream in;

			try
			{
				in = connSocket.getInputStream();
			}
			catch(IOException e)
			{
				return; // Stop receiver thread if input stream cannot be obtained
			}

			while(connSocket.isConnected())
			{
				try
				{
					final NXCPMessage msg = receiver.receiveMessage(in, encryptionContext);
					switch(msg.getMessageCode())
					{
						case NXCPCodes.CMD_REQUEST_SESSION_KEY:
							setupEncryption(msg);
							break;
						default:
							msgWaitQueue.putMessage(msg);
							break;
					}
				}
				catch(IOException e)
				{
					break; // Stop on read errors
				}
				catch(NXCPException e)
				{
					if (e.getErrorCode() == NXCPException.SESSION_CLOSED)
						break;
				}
				catch(MobileAgentException e)
				{
					if (e.getErrorCode() == RCC.ENCRYPTION_ERROR)
						break;
				}
			}
		}
	}

	/**
	 * @param address
	 * @param deviceId
	 * @param loginName
	 * @param password
	 */
	public Session(String address, String deviceId, String loginName, String password)
	{
		this.connAddress = address;
		this.connPort = DEFAULT_CONN_PORT;
		this.connDeviceId = deviceId;
		this.connLoginName = loginName;
		this.connPassword = password;
		this.connUseEncryption = false;
	}

	/**
	 * @param address
	 * @param port
	 * @param deviceId
	 * @param loginName
	 * @param password
	 */
	public Session(String address, int port, String deviceId, String loginName, String password)
	{
		this.connAddress = address;
		this.connPort = port;
		this.connDeviceId = deviceId;
		this.connLoginName = loginName;
		this.connPassword = password;
		this.connUseEncryption = false;
	}

	/**
	 * @param address
	 * @param port
	 * @param deviceId
	 * @param loginName
	 * @param password
	 * @param useEncryption
	 */
	public Session(String address, int port, String deviceId, String loginName, String password, boolean useEncryption)
	{
		this.connAddress = address;
		this.connPort = port;
		this.connDeviceId = deviceId;
		this.connLoginName = loginName;
		this.connPassword = password;
		this.connUseEncryption = useEncryption;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#finalize()
	 */
	@Override
	protected void finalize()
	{
		disconnect();
	}

	/**
	 * Send message to server
	 * 
	 * @param msg
	 *           Message to sent
	 * @throws IOException in case of socket communication failure
	 * @throws MobileAgentException in case of encryption error
	 */
	public synchronized void sendMessage(final NXCPMessage msg) throws IOException, MobileAgentException
	{
		if (connSocket == null)
		{
			throw new IllegalStateException("Not connected to the server. Did you forgot to call connect() first?");
		}
		final OutputStream outputStream = connSocket.getOutputStream();
		byte[] message;
		if ((encryptionContext != null) && !msg.isEncryptionDisabled())
		{
			try
			{
				message = encryptionContext.encryptMessage(msg);
			}
			catch(GeneralSecurityException e)
			{
				throw new MobileAgentException(RCC.ENCRYPTION_ERROR);
			}
		}
		else
		{
			message = msg.createNXCPMessage();
		}
		outputStream.write(message);
	}

	/**
	 * Wait for message with specific code and id.
	 * 
	 * @param code
	 * @param id
	 * @param timeout
	 * @return Message object
	 * @throws MobileAgentException
	 */
	public NXCPMessage waitForMessage(final int code, final long id, final int timeout) throws MobileAgentException
	{
		final NXCPMessage msg = msgWaitQueue.waitForMessage(code, id, timeout);
		if (msg == null)
			throw new MobileAgentException(RCC.TIMEOUT);
		return msg;
	}

	/**
	 * Wait for message with specific code and id.
	 * 
	 * @param code
	 * @param id
	 * @return Message object
	 * @throws MobileAgentException
	 */
	public NXCPMessage waitForMessage(final int code, final long id) throws MobileAgentException
	{
		final NXCPMessage msg = msgWaitQueue.waitForMessage(code, id);
		if (msg == null)
			throw new MobileAgentException(RCC.TIMEOUT);
		return msg;
	}

	/**
	 * Wait for CMD_REQUEST_COMPLETED message with given id using default timeout
	 * 
	 * @param id
	 * @return
	 * @throws MobileAgentException
	 */
	public NXCPMessage waitForRCC(final long id) throws MobileAgentException
	{
		return waitForRCC(id, msgWaitQueue.getDefaultTimeout());
	}

	/**
	 * @param id
	 * @param timeout
	 * @return
	 * @throws NXCException
	 */
	public NXCPMessage waitForRCC(final long id, final int timeout) throws MobileAgentException
	{
		final NXCPMessage msg = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, id, timeout);
		final int rcc = msg.getVariableAsInteger(NXCPCodes.VID_RCC);
		if (rcc != RCC.SUCCESS)
		{
			throw new MobileAgentException(rcc);
		}
		return msg;
	}
	
	/**
	 * Create new NXCP message with unique id
	 * 
	 * @param code message code
	 * @return new message object
	 */
	public final NXCPMessage newMessage(int code)
	{
		return new NXCPMessage(code, requestId.getAndIncrement());
	}

	/**
	 * Execute simple commands (without arguments and only returning RCC)
	 * 
	 * @param command
	 *           Command code
	 * @throws IOException
	 * @throws MobileAgentException
	 */
	protected void executeSimpleCommand(int command) throws IOException, MobileAgentException
	{
		final NXCPMessage msg = newMessage(command);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Connect to server.
	 * 
	 * @throws IOException
	 * @throws UnknownHostException
	 * @throws MobileAgentException
	 */
	public void connect() throws IOException, UnknownHostException, MobileAgentException
	{
		Logger.info("Session.connect", "Connecting to " + connAddress + ":" + connPort);
		try
		{
			connSocket = new Socket(connAddress, connPort);
			msgWaitQueue = new NXCPMsgWaitQueue(commandTimeout);
			recvThread = new ReceiverThread();

			// get server information
			Logger.debug("Session.connect", "connection established, retrieving server info");
			NXCPMessage request = newMessage(NXCPCodes.CMD_GET_SERVER_INFO);
			sendMessage(request);
			NXCPMessage response = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, request.getMessageId());

			if (response.getVariableAsInteger(NXCPCodes.VID_PROTOCOL_VERSION) != PROTOCOL_VERSION)
			{
				Logger.warning("Session.connect", "connection failed, server protocol version is " + response.getVariableAsInteger(NXCPCodes.VID_PROTOCOL_VERSION));
				throw new MobileAgentException(RCC.BAD_PROTOCOL);
			}
			
			String serverVersion = response.getVariableAsString(NXCPCodes.VID_SERVER_VERSION); 

			// Setup encryption if required
			if (connUseEncryption)
			{
				request = newMessage(NXCPCodes.CMD_REQUEST_ENCRYPTION);
				request.setVariableInt16(NXCPCodes.VID_USE_X509_KEY_FORMAT, 1);
				sendMessage(request);
				waitForRCC(request.getMessageId());
			}

			// Login to server
			Logger.debug("Session.connect", "Connected to server version " + serverVersion + ", trying to login");
			request = newMessage(NXCPCodes.CMD_LOGIN);
			request.setVariable(NXCPCodes.VID_DEVICE_ID, connDeviceId);
			request.setVariable(NXCPCodes.VID_LOGIN_NAME, connLoginName);
			request.setVariable(NXCPCodes.VID_PASSWORD, connPassword);
			request.setVariable(NXCPCodes.VID_LIBNXCL_VERSION, NXCommon.VERSION);
			request.setVariable(NXCPCodes.VID_OS_INFO, System.getProperty("os.name") + " " + System.getProperty("os.version"));
			sendMessage(request);
			response = waitForMessage(NXCPCodes.CMD_LOGIN_RESP, request.getMessageId());
			int rcc = response.getVariableAsInteger(NXCPCodes.VID_RCC);
			Logger.debug("Session.connect", "CMD_LOGIN_RESP received, RCC=" + rcc);
			if (rcc != RCC.SUCCESS)
			{
				Logger.warning("NXCSession.connect", "Login failed, RCC=" + rcc);
				throw new MobileAgentException(rcc);
			}

			Logger.info("Session.connect", "succesfully connected and logged in");
			isConnected = true;
		}
		finally
		{
			if (!isConnected)
				disconnect();
		}
	}

	/**
	 * Disconnect from server 
	 */
	public void disconnect()
	{
		if (connSocket != null)
		{
			try
			{
				connSocket.close();
			}
			catch(IOException e)
			{
			}
		}

		if (recvThread != null)
		{
			while(recvThread.isAlive())
			{
				try
				{
					recvThread.join();
				}
				catch(InterruptedException e)
				{
				}
			}
			recvThread = null;
		}

		connSocket = null;

		if (msgWaitQueue != null)
		{
			msgWaitQueue.shutdown();
			msgWaitQueue = null;
		}

		isConnected = false;
	}

	/**
	 * Set command execution timeout in milliseconds.
	 * 
	 * @param commandTimeout
	 */
	public void setCommandTimeout(final int commandTimeout)
	{
		this.commandTimeout = commandTimeout;
	}
	
	/**
	 * Push data to server.
	 * 
	 * @param data push data 
	 * @throws IOException if socket I/O error occurs
	 * @throws MobileAgentException if NetXMS server returns an error or operation was timed out
	 */
	public void pushDciData(DciPushData[] data) throws IOException, MobileAgentException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_PUSH_DCI_DATA);
		msg.setVariableInt32(NXCPCodes.VID_NUM_ITEMS, data.length);
		long varId = NXCPCodes.VID_PUSH_DCI_DATA_BASE;
		for(DciPushData d : data)
		{
			msg.setVariableInt32(varId++, (int)d.nodeId);
			if (d.nodeId == 0)
				msg.setVariable(varId++, d.nodeName);
			msg.setVariableInt32(varId++, (int)d.dciId);
			if (d.dciId == 0)
				msg.setVariable(varId++, d.dciName);
			msg.setVariable(varId++, d.value);
		}
		
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	/**
	 * Push value for single DCI.
	 * 
	 * @param nodeId node ID
	 * @param dciId DCI ID
	 * @param value value to push
	 * @throws IOException if socket I/O error occurs
	 * @throws MobileAgentException if NetXMS server returns an error or operation was timed out
	 */
	public void pushDciData(long nodeId, long dciId, String value) throws IOException, MobileAgentException
	{
		pushDciData(new DciPushData[] { new DciPushData(nodeId, dciId, value) });
	}

	/**
	 * Push value for single DCI.
	 * 
	 * @param nodeName node name
	 * @param dciName DCI name
	 * @param value value to push
	 * @throws IOException if socket I/O error occurs
	 * @throws MobileAgentException if NetXMS server returns an error or operation was timed out
	 */
	public void pushDciData(String nodeName, String dciName, String value) throws IOException, MobileAgentException
	{
		pushDciData(new DciPushData[] { new DciPushData(nodeName, dciName, value) });
	}
	
	/**
	 * Report basic system information about mobile device. Additional information may be reported via push DCIs.
	 * 
	 * @param vendor vendor name (like "HTC")
	 * @param model device model (like "Desire A8181")
	 * @param osName operating system name (like "Android")
	 * @param osVersion operating system version (like "2.2")
	 * @param serialNumber device serial number
	 * @param userId user id, if available (can be null)
	 * @throws IOException if socket I/O error occurs
	 * @throws MobileAgentException if NetXMS server returns an error or operation was timed out
	 */
	public void reportDeviceSystemInfo(String vendor, String model, String osName, String osVersion, String serialNumber, String userId) throws IOException, MobileAgentException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_REPORT_DEVICE_INFO);
		msg.setVariable(NXCPCodes.VID_VENDOR, vendor);
		msg.setVariable(NXCPCodes.VID_MODEL, model);
		msg.setVariable(NXCPCodes.VID_OS_NAME, osName);
		msg.setVariable(NXCPCodes.VID_OS_VERSION, osVersion);
		msg.setVariable(NXCPCodes.VID_SERIAL_NUMBER, serialNumber);
		if (userId != null)
			msg.setVariable(NXCPCodes.VID_USER_NAME, userId);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}
	
	/**
	 * Report basic current status of the device. Additional information may be reported via push DCIs.
	 * 
	 * @param address current IP address of the device (may be null if not known)
	 * @param location current device location (may be null if not known)
	 * @param flags
	 * @param batteryLevel current battery level, -1 if not known or not applicable
	 * @throws IOException if socket I/O error occurs
	 * @throws MobileAgentException if NetXMS server returns an error or operation was timed out
	 */
	public void reportDeviceStatus(InetAddress address, GeoLocation location, int flags, int batteryLevel) throws IOException, MobileAgentException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_REPORT_DEVICE_STATUS);
		
		if (address != null)
			msg.setVariable(NXCPCodes.VID_IP_ADDRESS, address);
		else
			msg.setVariableInt32(NXCPCodes.VID_IP_ADDRESS, 0);
		
		if (location != null)
		{
			msg.setVariable(NXCPCodes.VID_LATITUDE, location.getLatitude());
			msg.setVariable(NXCPCodes.VID_LONGITUDE, location.getLongitude());
			msg.setVariableInt16(NXCPCodes.VID_GEOLOCATION_TYPE, location.getType());
		}
		else
		{
			msg.setVariableInt16(NXCPCodes.VID_GEOLOCATION_TYPE, GeoLocation.UNSET);
		}
		
		msg.setVariableInt32(NXCPCodes.VID_FLAGS, flags);
		msg.setVariableInt16(NXCPCodes.VID_BATTERY_LEVEL, batteryLevel);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}
}
