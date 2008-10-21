/**
 *
 */
package org.netxms.client;

import java.io.*;
import java.net.*;
import java.util.concurrent.*;
import java.util.Map;
import java.util.HashMap;
import org.netxms.base.*;

/**
 * @author victor
 */
public class NXCSession
{
	// Various public constants
	public static final int DEFAULT_CONN_PORT = 4701;
	public static final int CLIENT_PROTOCOL_VERSION = 19;
	
	// Authentication types
	public static final int AUTH_TYPE_PASSWORD = 0;
	public static final int AUTH_TYPE_CERTIFICATE = 1;

	// Request completion codes (RCC)
	public static final int RCC_SUCCESS                 = 0;
	public static final int RCC_COMPONENT_LOCKED        = 1;
	public static final int RCC_ACCESS_DENIED           = 2;
	public static final int RCC_INVALID_REQUEST         = 3;
	public static final int RCC_TIMEOUT                 = 4;
	public static final int RCC_OUT_OF_STATE_REQUEST    = 5;
	public static final int RCC_DB_FAILURE              = 6;
	public static final int RCC_INVALID_OBJECT_ID       = 7;
	public static final int RCC_ALREADY_EXIST           = 8;
	public static final int RCC_COMM_FAILURE            = 9;
	public static final int RCC_SYSTEM_FAILURE          = 10;
	public static final int RCC_INVALID_USER_ID         = 11;
	public static final int RCC_INVALID_ARGUMENT        = 12;
	public static final int RCC_DUPLICATE_DCI           = 13;
	public static final int RCC_INVALID_DCI_ID          = 14;
	public static final int RCC_OUT_OF_MEMORY           = 15;
	public static final int RCC_IO_ERROR                = 16;
	public static final int RCC_INCOMPATIBLE_OPERATION  = 17;
	public static final int RCC_OBJECT_CREATION_FAILED  = 18;
	public static final int RCC_OBJECT_LOOP             = 19;
	public static final int RCC_INVALID_OBJECT_NAME     = 20;
	public static final int RCC_INVALID_ALARM_ID        = 21;
	public static final int RCC_INVALID_ACTION_ID       = 22;
	public static final int RCC_OPERATION_IN_PROGRESS   = 23;
	public static final int RCC_DCI_COPY_ERRORS         = 24;
	public static final int RCC_INVALID_EVENT_CODE      = 25;
	public static final int RCC_NO_WOL_INTERFACES       = 26;
	public static final int RCC_NO_MAC_ADDRESS          = 27;
	public static final int RCC_NOT_IMPLEMENTED         = 28;
	public static final int RCC_INVALID_TRAP_ID         = 29;
	public static final int RCC_DCI_NOT_SUPPORTED       = 30;
	public static final int RCC_VERSION_MISMATCH        = 31;
	public static final int RCC_NPI_PARSE_ERROR         = 32;
	public static final int RCC_DUPLICATE_PACKAGE       = 33;
	public static final int RCC_PACKAGE_FILE_EXIST      = 34;
	public static final int RCC_RESOURCE_BUSY           = 35;
	public static final int RCC_INVALID_PACKAGE_ID      = 36;
	public static final int RCC_INVALID_IP_ADDR         = 37;
	public static final int RCC_ACTION_IN_USE           = 38;
	public static final int RCC_VARIABLE_NOT_FOUND      = 39;
	public static final int RCC_BAD_PROTOCOL            = 40;
	public static final int RCC_ADDRESS_IN_USE          = 41;
	public static final int RCC_NO_CIPHERS              = 42;
	public static final int RCC_INVALID_PUBLIC_KEY      = 43;
	public static final int RCC_INVALID_SESSION_KEY     = 44;
	public static final int RCC_NO_ENCRYPTION_SUPPORT   = 45;
	public static final int RCC_INTERNAL_ERROR          = 46;
	public static final int RCC_EXEC_FAILED             = 47;
	public static final int RCC_INVALID_TOOL_ID         = 48;
	public static final int RCC_SNMP_ERROR              = 49;
	public static final int RCC_BAD_REGEXP              = 50;
	public static final int RCC_UNKNOWN_PARAMETER       = 51;
	public static final int RCC_FILE_IO_ERROR           = 52;
	public static final int RCC_CORRUPTED_MIB_FILE      = 53;
	public static final int RCC_TRANSFER_IN_PROGRESS    = 54;
	public static final int RCC_INVALID_LPP_ID          = 55;
	public static final int RCC_INVALID_SCRIPT_ID       = 56;
	public static final int RCC_INVALID_SCRIPT_NAME     = 57;
	public static final int RCC_UNKNOWN_MAP_NAME        = 58;
	public static final int RCC_INVALID_MAP_ID          = 59;
	public static final int RCC_ACCOUNT_DISABLED        = 60;
	public static final int RCC_NO_GRACE_LOGINS         = 61;
	public static final int RCC_CONNECTION_BROKEN       = 62;
	public static final int RCC_INVALID_CONFIG_ID       = 63;
	public static final int RCC_DB_CONNECTION_LOST      = 64;
	public static final int RCC_ALARM_OPEN_IN_HELPDESK  = 65;
	public static final int RCC_ALARM_NOT_OUTSTANDING   = 66;
	public static final int RCC_NOT_PUSH_DCI            = 67;
	public static final int RCC_NXMP_PARSE_ERROR        = 68;
	public static final int RCC_NXMP_VALIDATION_ERROR   = 69;
	public static final int RCC_INVALID_GRAPH_ID        = 70;
	public static final int RCC_LOCAL_CRYPTO_ERROR      = 71;
	public static final int RCC_UNSUPPORTED_AUTH_TYPE   = 72;
	public static final int RCC_BAD_CERTIFICATE         = 73;
	public static final int RCC_INVALID_CERT_ID         = 74;
	public static final int RCC_SNMP_FAILURE            = 75;
	public static final int RCC_NO_L2_TOPOLOGY_SUPPORT  = 76;
	public static final int RCC_INVALID_SITUATION_ID    = 77;
	public static final int RCC_INSTANCE_NOT_FOUND      = 78;
	public static final int RCC_INVALID_EVENT_ID        = 79;
	public static final int RCC_AGENT_ERROR             = 80;
	public static final int RCC_UNKNOWN_VARIABLE        = 81;
	public static final int RCC_RESOURCE_NOT_AVAILABLE  = 82;	
	
	// Private constants
	private static final int CLIENT_CHALLENGE_SIZE = 256;
	
	// Internal synchronization objects
	private final Semaphore syncObjects = new Semaphore(1);
	
	// Connection-related attributes
	private String connAddress;
	private int connPort;
	private String connLoginName;
	private String connPassword;
	private boolean connUseEncryption;
	private String connClientInfo = "nxjclient/" + NetXMSConst.VERSION;
	
	// Information about logged in user
	private int userId;
	private int userSystemRights;
	
	// Internal communication data
	private Socket connSocket = null;
	private NXCPMsgWaitQueue msgWaitQueue = null;
	private ReceiverThread recvThread = null;
	private long requestId = 0;
	private boolean isConnected = false;
	
	// Communication parameters
	private int recvBufferSize = 4194304;	// Default is 4MB
	private int commandTimeout = 30000;		// Default is 30 sec
	
	// Server information
	private String serverVersion = "(unknown)";
	private byte[] serverId = new byte[8];
	private String serverTimeZone;
	private byte[] serverChallenge = new byte[CLIENT_CHALLENGE_SIZE];
	
	// Objects
	private Map<Long, NXCObject> objectList = new HashMap<Long, NXCObject>();
	
	
	//
	// Create object from message
	//
	
	private static NXCObject createObjectFromMessage(NXCPMessage msg)
	{
		final int objectClass = msg.getVariableAsInteger(NXCPCodes.VID_OBJECT_CLASS);
		NXCObject object;
		
		switch(objectClass)
		{
			case NXCObject.OBJECT_INTERFACE:
				object = new NXCInterface(msg);
				break;
			case NXCObject.OBJECT_SUBNET:
				object = new NXCSubnet(msg);
				break;
			case NXCObject.OBJECT_CONTAINER:
				object = new NXCContainer(msg);
				break;
			case NXCObject.OBJECT_NODE:
				object = new NXCNode(msg);
				break;
			default:
				object = new NXCObject(msg);
				break;
		}
		
		return object;
	}
	
	
	//
	// Receiver thread
	//
	
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
				return;	// Stop receiver thread if input stream cannot be obtained
			}
			
			while(connSocket.isConnected())
			{
				try
				{
					final NXCPMessage msg = receiver.receiveMessage(in);
					switch(msg.getMessageCode())
					{
						case NXCPCodes.CMD_OBJECT:
						case NXCPCodes.CMD_OBJECT_UPDATE:
							final NXCObject obj = createObjectFromMessage(msg);
							synchronized(objectList)
							{
								objectList.put(obj.getObjectId(), obj);
							}
							break;
						case NXCPCodes.CMD_OBJECT_LIST_END:
							completeSync(syncObjects);
							break;
						default:
							msgWaitQueue.putMessage(msg);
							break;
					}
				}
				catch(IOException e)
				{
					break;	// Stop on read errors
				}
				catch(NXCPException e)
				{
				}
			}
		}
	}
	
	
	//
	// Constructors
	//
	
	/**
	 * @param connAddress
	 * @param connLoginName
	 * @param connPassword
	 */
	public NXCSession(String connAddress, String connLoginName, String connPassword)
	{
		this.connAddress = connAddress;
		this.connPort = DEFAULT_CONN_PORT;
		this.connLoginName = connLoginName;
		this.connPassword = connPassword;
		this.connUseEncryption = false;
	}

	/**
	 * @param connAddress
	 * @param connPort
	 * @param connLoginName
	 * @param connPassword
	 */
	public NXCSession(String connAddress, int connPort, String connLoginName, String connPassword)
	{
		this.connAddress = connAddress;
		this.connPort = connPort;
		this.connLoginName = connLoginName;
		this.connPassword = connPassword;
		this.connUseEncryption = false;
	}

	/**
	 * @param connAddress
	 * @param connPort
	 * @param connLoginName
	 * @param connPassword
	 * @param connUseEncryption
	 */
	public NXCSession(String connAddress, int connPort, String connLoginName, String connPassword, boolean connUseEncryption)
	{
		this.connAddress = connAddress;
		this.connPort = connPort;
		this.connLoginName = connLoginName;
		this.connPassword = connPassword;
		this.connUseEncryption = connUseEncryption;
	}

	
	//
	// Finalize
	//
	
	@Override
	protected void finalize()
	{
		disconnect();
	}
	
	
	//
	// Wait for synchronization
	//
	
	private void waitForSync(final Semaphore syncObject, final int timeout) throws NXCException
	{
		if (timeout == 0)
		{
			syncObject.acquireUninterruptibly();
		}
		else
		{
			long actualTimeout = timeout;
			boolean success = false;
			
			while(actualTimeout > 0)
			{
				long startTime = System.currentTimeMillis();
				try
				{
					if (syncObjects.tryAcquire(actualTimeout, TimeUnit.MILLISECONDS))
					{
						success = true;
						break;
					}
				}
				catch(InterruptedException e)
				{
				}
				actualTimeout -= System.currentTimeMillis() - startTime;
			}
			if (!success)
				throw new NXCException(RCC_TIMEOUT);
		}
	}
	
	
	//
	// Report synchronization completion
	//
	
	private void completeSync(final Semaphore syncObject)
	{
		syncObject.release();
	}
	
	
	//
	// Send message to server
	//
	
	private synchronized void sendMessage(final NXCPMessage msg) throws IOException
	{
		connSocket.getOutputStream().write(msg.createNXCPMessage());
	}
	
	
	//
	// Wait for message
	//
	
	private NXCPMessage waitForMessage(final int code, final long id) throws NXCException
	{
		final NXCPMessage msg = msgWaitQueue.waitForMessage(code, id);
		if (msg == null)
			throw new NXCException(RCC_TIMEOUT);
		return msg;
	}
	
	
	//
	// Wait for CMD_REQUEST_COMPLETED
	//
	
	private void waitForRCC(final long id) throws NXCException
	{
		final NXCPMessage msg = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, id);
		final int rcc = msg.getVariableAsInteger(NXCPCodes.VID_RCC);
		if (rcc != RCC_SUCCESS)
			throw new NXCException(rcc);
	}
	
	
	//
	// Create new message with unique id
	//
	
	private final NXCPMessage newMessage(int code)
	{
		return new NXCPMessage(code, requestId++);
	}

	
	//
	// Connect to server
	//
	
	public void connect() throws IOException, UnknownHostException, NXCException
	{
		try
		{
			connSocket = new Socket(connAddress, connPort);
			msgWaitQueue = new NXCPMsgWaitQueue(commandTimeout);
			recvThread = new ReceiverThread();
			
			// get server information
			NXCPMessage request = newMessage(NXCPCodes.CMD_GET_SERVER_INFO);
			sendMessage(request);
			NXCPMessage response = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, request.getMessageId());
			
			if (response.getVariableAsInteger(NXCPCodes.VID_PROTOCOL_VERSION) != CLIENT_PROTOCOL_VERSION)
				throw new NXCException(RCC_BAD_PROTOCOL);
			
			serverVersion = response.getVariableAsString(NXCPCodes.VID_SERVER_VERSION);
			serverId = response.getVariableAsBinary(NXCPCodes.VID_SERVER_ID);
			serverTimeZone = response.getVariableAsString(NXCPCodes.VID_TIMEZONE);
			serverChallenge = response.getVariableAsBinary(NXCPCodes.VID_CHALLENGE);
			
			// Login to server
			request = newMessage(NXCPCodes.CMD_LOGIN);
			request.setVariable(NXCPCodes.VID_LOGIN_NAME, connLoginName);
			request.setVariable(NXCPCodes.VID_PASSWORD, connPassword);
			request.setVariableInt16(NXCPCodes.VID_AUTH_TYPE, AUTH_TYPE_PASSWORD);
			request.setVariable(NXCPCodes.VID_LIBNXCL_VERSION, NetXMSConst.VERSION);
			request.setVariable(NXCPCodes.VID_CLIENT_INFO, connClientInfo);
			request.setVariable(NXCPCodes.VID_OS_INFO, System.getProperty("os.name") + " " + System.getProperty("os.version"));
			sendMessage(request);
			response = waitForMessage(NXCPCodes.CMD_LOGIN_RESP, request.getMessageId());
			int rcc = response.getVariableAsInteger(NXCPCodes.VID_RCC);
			if (rcc != RCC_SUCCESS)
				throw new NXCException(rcc);
			userId = response.getVariableAsInteger(NXCPCodes.VID_USER_ID);
			userSystemRights = response.getVariableAsInteger(NXCPCodes.VID_USER_SYS_RIGHTS);
			
			isConnected = true;
		}
		finally
		{
			if (!isConnected)
				disconnect();
		}
	}
	
	
	//
	// Disconnect
	//
	
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
	 * @return the recvBufferSize
	 */
	public int getRecvBufferSize()
	{
		return recvBufferSize;
	}

	/**
	 * @param recvBufferSize the recvBufferSize to set
	 */
	public void setRecvBufferSize(int recvBufferSize)
	{
		this.recvBufferSize = recvBufferSize;
	}

	/**
	 * @return the serverVersion
	 */
	public String getServerVersion()
	{
		return serverVersion;
	}

	/**
	 * @return the serverId
	 */
	public byte[] getServerId()
	{
		return serverId;
	}

	/**
	 * @return the serverTimeZone
	 */
	public String getServerTimeZone()
	{
		return serverTimeZone;
	}

	/**
	 * @return the serverChallenge
	 */
	public byte[] getServerChallenge()
	{
		return serverChallenge;
	}

	/**
	 * @return the connClientInfo
	 */
	public String getConnClientInfo()
	{
		return connClientInfo;
	}

	/**
	 * @param connClientInfo the connClientInfo to set
	 */
	public void setConnClientInfo(final String connClientInfo)
	{
		this.connClientInfo = connClientInfo;
	}

	/**
	 * @param commandTimeout New command timeout
	 */
	public void setCommandTimeout(final int commandTimeout)
	{
		this.commandTimeout = commandTimeout;
	}

	/**
	 * @return the userId
	 */
	public int getUserId()
	{
		return userId;
	}

	/**
	 * @return the userSystemRights
	 */
	public int getUserSystemRights()
	{
		return userSystemRights;
	}
	
	
	//
	// Synchronize objects
	//
	
	public synchronized void syncObjects() throws IOException, NXCException
	{
		syncObjects.acquireUninterruptibly();
		NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_OBJECTS);
		msg.setVariableInt16(NXCPCodes.VID_SYNC_COMMENTS, 1);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
		waitForSync(syncObjects, commandTimeout * 10);
	}
	
	
	//
	// Find object by ID
	//
	
	public NXCObject findObjectById(final long id)
	{
		NXCObject obj;
		
		synchronized(objectList)
		{
			obj = objectList.get(id);
		}
		return obj;
	}
}
