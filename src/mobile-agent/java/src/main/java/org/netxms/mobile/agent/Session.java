/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPException;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCPMessageReceiver;
import org.netxms.base.NXCPMsgWaitQueue;
import org.netxms.base.VersionInfo;
import org.netxms.mobile.agent.constants.RCC;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Communication session with NetXMS server.
 */
public class Session
{
   public static final Logger logger = LoggerFactory.getLogger(Session.class);

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
	private boolean allowCompression = false;

	// Communication parameters
	private int defaultRecvBufferSize = 65536; // Default is 64KB
   private int maxRecvBufferSize = 262144;    // Max is 256KB
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
			response.setField(NXCPCodes.VID_SESSION_KEY, encryptionContext.getEncryptedSessionKey());
			response.setField(NXCPCodes.VID_SESSION_IV, encryptionContext.getEncryptedIv());
			response.setFieldInt16(NXCPCodes.VID_CIPHER, encryptionContext.getCipher());
			response.setFieldInt16(NXCPCodes.VID_KEY_LENGTH, encryptionContext.getKeyLength());
			response.setFieldInt16(NXCPCodes.VID_IV_LENGTH, encryptionContext.getIvLength());
			response.setFieldInt32(NXCPCodes.VID_RCC, RCC.SUCCESS);
         logger.debug("Cipher selected: " + EncryptionContext.getCipherName(encryptionContext.getCipher()));
		}
		catch(Exception e)
		{
			response.setFieldInt32(NXCPCodes.VID_RCC, RCC.NO_CIPHERS);
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
			final NXCPMessageReceiver receiver = new NXCPMessageReceiver(defaultRecvBufferSize, maxRecvBufferSize);
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
				message = encryptionContext.encryptMessage(msg, allowCompression);
			}
			catch(GeneralSecurityException e)
			{
				throw new MobileAgentException(RCC.ENCRYPTION_ERROR);
			}
		}
		else
		{
			message = msg.createNXCPMessage(allowCompression);
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
	 * @param code message code
	 * @param id message ID
	 * @return Message object
    * @throws MobileAgentException if server returns an error or request was timed out
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
	 * @param id message ID
	 * @return received message
    * @throws MobileAgentException if server returns an error or request was timed out
	 */
	public NXCPMessage waitForRCC(final long id) throws MobileAgentException
	{
		return waitForRCC(id, msgWaitQueue.getDefaultTimeout());
	}

	/**
	 * Wait for request completion message.
	 * 
	 * @param id message ID
	 * @param timeout timeout (milliseconds)
	 * @return received message
	 * @throws MobileAgentException if server returns an error or request was timed out
	 */
	public NXCPMessage waitForRCC(final long id, final int timeout) throws MobileAgentException
	{
		final NXCPMessage msg = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, id, timeout);
		final int rcc = msg.getFieldAsInt32(NXCPCodes.VID_RCC);
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
    * @throws MobileAgentException if server returns an error or request was timed out
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
    * @throws MobileAgentException if server returns an error or request was timed out
	 */
	public void connect() throws IOException, UnknownHostException, MobileAgentException
	{
      logger.info("Connecting to " + connAddress + ":" + connPort);
		try
		{
			connSocket = new Socket(connAddress, connPort);
			msgWaitQueue = new NXCPMsgWaitQueue(commandTimeout);
			recvThread = new ReceiverThread();

			// get server information
         logger.debug("Connection established, retrieving server info");
			NXCPMessage request = newMessage(NXCPCodes.CMD_GET_SERVER_INFO);
			sendMessage(request);
			NXCPMessage response = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, request.getMessageId());

			if (response.getFieldAsInt32(NXCPCodes.VID_PROTOCOL_VERSION) != PROTOCOL_VERSION)
			{
            logger.warn(
                  "Connection failed, server protocol version is " + response.getFieldAsInt32(NXCPCodes.VID_PROTOCOL_VERSION));
				throw new MobileAgentException(RCC.BAD_PROTOCOL);
			}
			
			String serverVersion = response.getFieldAsString(NXCPCodes.VID_SERVER_VERSION); 

			// Setup encryption if required
			if (connUseEncryption)
			{
				request = newMessage(NXCPCodes.CMD_REQUEST_ENCRYPTION);
				request.setFieldInt16(NXCPCodes.VID_USE_X509_KEY_FORMAT, 1);
				sendMessage(request);
				waitForRCC(request.getMessageId());
			}

			// Login to server
         logger.debug("Connected to server version " + serverVersion + ", trying to login");
			request = newMessage(NXCPCodes.CMD_LOGIN);
			request.setField(NXCPCodes.VID_DEVICE_ID, connDeviceId);
			request.setField(NXCPCodes.VID_LOGIN_NAME, connLoginName);
			request.setField(NXCPCodes.VID_PASSWORD, connPassword);
			request.setField(NXCPCodes.VID_LIBNXCL_VERSION, VersionInfo.version());
			request.setField(NXCPCodes.VID_OS_INFO, System.getProperty("os.name") + " " + System.getProperty("os.version"));
			sendMessage(request);
			response = waitForMessage(NXCPCodes.CMD_LOGIN_RESPONSE, request.getMessageId());
			int rcc = response.getFieldAsInt32(NXCPCodes.VID_RCC);
         logger.debug("CMD_LOGIN_RESP received, RCC=" + rcc);
			if (rcc != RCC.SUCCESS)
			{
            logger.warn("Login failed, RCC=" + rcc);
				throw new MobileAgentException(rcc);
			}

         logger.info("Succesfully connected and logged in");
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
				connSocket.shutdownInput();
				connSocket.shutdownOutput();
			}
			catch(IOException e)
			{
			}
			
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
		msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, data.length);
		long varId = NXCPCodes.VID_PUSH_DCI_DATA_BASE;
		for(DciPushData d : data)
		{
			msg.setFieldInt32(varId++, (int)d.nodeId);
			if (d.nodeId == 0)
				msg.setField(varId++, d.nodeName);
			msg.setFieldInt32(varId++, (int)d.dciId);
			if (d.dciId == 0)
				msg.setField(varId++, d.dciName);
			msg.setField(varId++, d.value);
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
		msg.setField(NXCPCodes.VID_VENDOR, vendor);
		msg.setField(NXCPCodes.VID_MODEL, model);
		msg.setField(NXCPCodes.VID_OS_NAME, osName);
		msg.setField(NXCPCodes.VID_OS_VERSION, osVersion);
		msg.setField(NXCPCodes.VID_SERIAL_NUMBER, serialNumber);
		if (userId != null)
			msg.setField(NXCPCodes.VID_USER_NAME, userId);
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
			msg.setField(NXCPCodes.VID_IP_ADDRESS, address);
		else
			msg.setFieldInt32(NXCPCodes.VID_IP_ADDRESS, 0);
		
		if (location != null)
		{
			msg.setField(NXCPCodes.VID_LATITUDE, location.getLatitude());
			msg.setField(NXCPCodes.VID_LONGITUDE, location.getLongitude());
			msg.setFieldInt16(NXCPCodes.VID_GEOLOCATION_TYPE, location.getType());
			msg.setFieldInt16(NXCPCodes.VID_ACCURACY, location.getAccuracy());
			msg.setFieldInt64(NXCPCodes.VID_GEOLOCATION_TIMESTAMP, location.getTimestamp() != null ? location.getTimestamp().getTime() / 1000 : 0);
		}
		else
		{
			msg.setFieldInt16(NXCPCodes.VID_GEOLOCATION_TYPE, GeoLocation.UNSET);
		}
		
		msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
		msg.setFieldInt32(NXCPCodes.VID_BATTERY_LEVEL, batteryLevel);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}
}
