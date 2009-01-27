/**
 *
 */
package org.netxms.client;

import java.io.*;
import java.net.*;
import java.util.concurrent.*;
import java.util.HashSet;
import java.util.Iterator;
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
	
	// Notification channels
	public static final int CHANNEL_EVENTS = 0x0001;
	public static final int CHANNEL_SYSLOG = 0x0002;
	public static final int CHANNEL_ALARMS = 0x0004;
	public static final int CHANNEL_OBJECTS = 0x0008;
	public static final int CHANNEL_SNMP_TRAPS = 0x0010;
	public static final int CHANNEL_AUDIT_LOG = 0x0020;
	public static final int CHANNEL_SITUATIONS = 0x0040;
	
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
	
	// Notification listeners
	private HashSet<NXCListener> listeners = new HashSet<NXCListener>(0);
	
	// Server information
	private String serverVersion = "(unknown)";
	private byte[] serverId = new byte[8];
	private String serverTimeZone;
	private byte[] serverChallenge = new byte[CLIENT_CHALLENGE_SIZE];
	
	// Objects
	private Map<Long, NXCObject> objectList = new HashMap<Long, NXCObject>();
	
	
	/**
	 * Create object from message
	 * 
	 * @param msg Source NXCP message
	 * @return NetXMS object
	 */
	private NXCObject createObjectFromMessage(NXCPMessage msg)
	{
		final int objectClass = msg.getVariableAsInteger(NXCPCodes.VID_OBJECT_CLASS);
		NXCObject object;
		
		switch(objectClass)
		{
			case NXCObject.OBJECT_INTERFACE:
				object = new NXCInterface(msg, this);
				break;
			case NXCObject.OBJECT_SUBNET:
				object = new NXCSubnet(msg, this);
				break;
			case NXCObject.OBJECT_CONTAINER:
				object = new NXCContainer(msg, this);
				break;
			case NXCObject.OBJECT_NODE:
				object = new NXCNode(msg, this);
				break;
			case NXCObject.OBJECT_NETWORK:
				object = new NXCEntireNetwork(msg, this);
				break;
			case NXCObject.OBJECT_SERVICEROOT:
				object = new NXCServiceRoot(msg, this);
				break;
			default:
				object = new NXCObject(msg, this);
				break;
		}
		
		return object;
	}
	

	/**
	 * Receiver thread for NXCSession
	 * @author victor
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
							if (msg.getMessageCode() == NXCPCodes.CMD_OBJECT_UPDATE)
								sendNotification(new NXCNotification(NXCNotification.OBJECT_CHANGED, obj));
							break;
						case NXCPCodes.CMD_OBJECT_LIST_END:
							completeSync(syncObjects);
							break;
						case NXCPCodes.CMD_ALARM_UPDATE:
							sendNotification(new NXCNotification(msg.getVariableAsInteger(NXCPCodes.VID_NOTIFICATION_CODE) + NXCNotification.NOTIFY_BASE,
							                                     new NXCAlarm(msg)));
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
						syncObjects.release();
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
	
	
	/**
	 * Report synchronization completion
	 * @param syncObject Synchronization object
	 */
	private void completeSync(final Semaphore syncObject)
	{
		syncObject.release();
	}
	
	
	/**
	 * Add notification listener
	 * 
	 * @param lst Listener to add
	 */
	public void addListener(NXCListener lst)
	{
		listeners.add(lst);
	}
	
	
	/**
	 * Remove notification listener
	 * 
	 * @param lst Listener to remove
	 */
	public void removeListener(NXCListener lst)
	{
		listeners.remove(lst);
	}
	
	
	/**
	 * Call notification handlers on all registered listeners
	 * @param n Notification object
	 */
	protected synchronized void sendNotification(NXCNotification n)
	{
		Iterator<NXCListener> it = listeners.iterator();
		while(it.hasNext())
		{
			it.next().notificationHandler(n);
		}
	}
	
	
	/**
	 * Send message to server
	 * @param msg Message to sent
	 * @throws IOException if case of socket communication failure
	 */
	private synchronized void sendMessage(final NXCPMessage msg) throws IOException
	{
		connSocket.getOutputStream().write(msg.createNXCPMessage());
	}
	

	/**
	 * Wait for message with specific code and id.
	 *  
	 * @param code Message code
	 * @param id Message id
	 * @return Message object 
	 * @throws NXCException if message was not arrived within timeout interval
	 */
	private NXCPMessage waitForMessage(final int code, final long id) throws NXCException
	{
		final NXCPMessage msg = msgWaitQueue.waitForMessage(code, id);
		if (msg == null)
			throw new NXCException(RCC_TIMEOUT);
		return msg;
	}
	

	/**
	 * Wait for CMD_REQUEST_COMPLETED message with given id
	 * 
	 * @param id Message id
	 * @throws NXCException if message was not arrived within timeout interval or contains
	 *         RCC other than RCC_SUCCESS
	 */
	private void waitForRCC(final long id) throws NXCException
	{
		final NXCPMessage msg = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, id);
		final int rcc = msg.getVariableAsInteger(NXCPCodes.VID_RCC);
		if (rcc != RCC_SUCCESS)
			throw new NXCException(rcc);
	}
	

	/**
	 * Create new NXCP message with unique id
	 * 
	 * @param code Message code
	 * @return New message object
	 */
	private final NXCPMessage newMessage(int code)
	{
		return new NXCPMessage(code, requestId++);
	}

	
	/**
	 * Connect to server using previously set credentials.
	 * 
	 * @throws IOException to indicate socket I/O error
	 * @throws UnknownHostException if given host name cannot be resolved
	 * @throws NXCException if NetXMS server returns an error, protocol
	 *                      negotiation with the server was failed, or operation 
	 *                      was timed out
	 */
	
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
	
	
	/**
	 * Disconnect from server.
	 * 
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
	 * Get receiver buffer size.
	 * 
	 * @return Current receiver buffer size in bytes.
	 */
	public int getRecvBufferSize()
	{
		return recvBufferSize;
	}

	/**
	 * Set receiver buffer size. This method should be called before connect().
	 * It will not have any effect after connect(). 
	 * 
	 * @param recvBufferSize Size of receiver buffer in bytes.
	 */
	public void setRecvBufferSize(int recvBufferSize)
	{
		this.recvBufferSize = recvBufferSize;
	}

	/**
	 * Get NetXMS server version.
	 * 
	 * @return Server version
	 */
	public String getServerVersion()
	{
		return serverVersion;
	}

	/**
	 * Get NetXMS server UID.
	 * 
	 * @return Server UID
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
	 * Set command execution timeout.
	 * 
	 * @param commandTimeout New command timeout
	 */
	public void setCommandTimeout(final int commandTimeout)
	{
		this.commandTimeout = commandTimeout;
	}

	/**
	 * Get identifier of logged in user.
	 * 
	 * @return Identifier of logged in user
	 */
	public int getUserId()
	{
		return userId;
	}

	/**
	 * Get system-wide rights of currently logged in user.
	 * 
	 * @return System-wide rights of currently logged in user
	 */

	public int getUserSystemRights()
	{
		return userSystemRights;
	}
	
	
	/**
	 * Synchronizes NetXMS objects between server and client.
	 * 
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	
	public synchronized void syncObjects() throws IOException, NXCException
	{
		syncObjects.acquireUninterruptibly();
		NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_OBJECTS);
		msg.setVariableInt16(NXCPCodes.VID_SYNC_COMMENTS, 1);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
		waitForSync(syncObjects, commandTimeout * 10);
	}
	
	
	/**
	 * Find NetXMS object by it's identifier.
	 * 
	 * @param id Object identifier
	 * @return Object with given ID or null if object cannot be found
	 */
	
	public NXCObject findObjectById(final long id)
	{
		NXCObject obj;
		
		synchronized(objectList)
		{
			obj = objectList.get(id);
		}
		return obj;
	}

	
	/**
	 * Get list of top-level objects.
	 * 
	 * @param name variable's name
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	
	public NXCObject[] getTopLevelObjects()
	{
		HashSet<NXCObject> list = new HashSet<NXCObject>();
		list.add(findObjectById(1));
		list.add(findObjectById(2));
		list.add(findObjectById(3));
		return list.toArray(new NXCObject[list.size()]);
	}

	
	/**
	 * Get list of all objects
	 */
	public NXCObject[] getAllObjects()
	{
		NXCObject[] list;
		
		synchronized(objectList)
		{
			list = objectList.values().toArray(new NXCObject[objectList.size()]);
		}
		return list;
	}

	
	/**
	 * Get alarm list.
	 * 
	 * @param getTerminated if set to true, all alarms will be retrieved from database,
	 *                      otherwise only active alarms
	 * @return Hash map containing alarms
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	
	public HashMap<Long, NXCAlarm> getAlarms(final boolean getTerminated) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_ALL_ALARMS);
		final long rqId = msg.getMessageId();
		
		msg.setVariableInt16(NXCPCodes.VID_IS_ACK, getTerminated ? 1 : 0);
		sendMessage(msg);
		
		final HashMap<Long, NXCAlarm> alarmList = new HashMap<Long, NXCAlarm>(0);
		while(true)
		{
			msg = waitForMessage(NXCPCodes.CMD_ALARM_DATA, rqId);
			long alarmId = msg.getVariableAsInteger(NXCPCodes.VID_ALARM_ID);
			if (alarmId == 0)
				break;	// ALARM_ID == 0 indicates end of list
			alarmList.put(alarmId, new NXCAlarm(msg));
		}
		
		return alarmList;
	}

	
	/**
	 * Acknowledge alarm.
	 * 
	 * @param alarmId Identifier of alarm to be acknowledged.
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	
	public void acknowledgeAlarm(final long alarmId) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_ACK_ALARM);
		msg.setVariableInt32(NXCPCodes.VID_ALARM_ID, (int)alarmId);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	
	/**
	 * Terminate alarm.
	 * 
	 * @param alarmId Identifier of alarm to be terminated.
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	
	public void terminateAlarm(final long alarmId) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_TERMINATE_ALARM);
		msg.setVariableInt32(NXCPCodes.VID_ALARM_ID, (int)alarmId);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	
	/**
	 * Delete alarm.
	 * 
	 * @param alarmId Identifier of alarm to be deleted.
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	
	public void deleteAlarm(final long alarmId) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_ALARM);
		msg.setVariableInt32(NXCPCodes.VID_ALARM_ID, (int)alarmId);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	
	/**
	 * Set alarm's helpdesk state to "Open".
	 * 
	 * @param alarmId Identifier of alarm to be changed.
	 * @param reference Helpdesk reference string.
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	
	public void openAlarm(final long alarmId, final String reference) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_ALARM_HD_STATE);
		msg.setVariableInt32(NXCPCodes.VID_ALARM_ID, (int)alarmId);
		msg.setVariableInt16(NXCPCodes.VID_HELPDESK_STATE, NXCAlarm.HELPDESK_STATE_OPEN);
		msg.setVariable(NXCPCodes.VID_HELPDESK_REF, reference);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}

	
	/**
	 * Set alarm's helpdesk state to "Closed".
	 * 
	 * @param alarmId Identifier of alarm to be changed.
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	
	public void closeAlarm(final long alarmId) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_ALARM_HD_STATE);
		msg.setVariableInt32(NXCPCodes.VID_ALARM_ID, (int)alarmId);
		msg.setVariableInt16(NXCPCodes.VID_HELPDESK_STATE, NXCAlarm.HELPDESK_STATE_CLOSED);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}
	
	
	/**
	 * Get server configuration variables.
	 * 
	 * @return Hash map containing server configuration variables
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	
	public HashMap<String, NXCServerVariable> getServerVariables() throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_GET_CONFIG_VARLIST);
		sendMessage(msg);

		msg = waitForMessage(NXCPCodes.CMD_REQUEST_COMPLETED, msg.getMessageId());
		int rcc = msg.getVariableAsInteger(NXCPCodes.VID_RCC);
		if (rcc != RCC_SUCCESS)
			throw new NXCException(rcc);
		
		long id;
		int i, count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_VARIABLES);
		final HashMap<String, NXCServerVariable> varList = new HashMap<String, NXCServerVariable>(count);
		for(i = 0, id = NXCPCodes.VID_VARLIST_BASE; i < count; i++, id += 3)
		{
			String name = msg.getVariableAsString(id);
			varList.put(name, new NXCServerVariable(name, msg.getVariableAsString(id + 1),
			                                        msg.getVariableAsBoolean(id + 2)));
		}
		
		return varList;
	}


	/**
	 * Set server configuration variable.
	 * 
	 * @param name variable's name
	 * @param value new variable's value
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void setServerVariable(final String name, final String value) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_SET_CONFIG_VARIABLE);
		msg.setVariable(NXCPCodes.VID_NAME, name);
		msg.setVariable(NXCPCodes.VID_VALUE, value);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}


	/**
	 * Delete server configuration variable.
	 * 
	 * @param name variable's name
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void deleteServerVariable(final String name) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_DELETE_CONFIG_VARIABLE);
		msg.setVariable(NXCPCodes.VID_NAME, name);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}
	

	/**
	 * Subscribe to notification channel(s)
	 * 
	 * @param channels Notification channels to subscribe to. Multiple channels can be specified by combining them with OR operation.
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void subscribe(int channels) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_CHANGE_SUBSCRIPTION);
		msg.setVariableInt32(NXCPCodes.VID_FLAGS, channels);
		msg.setVariableInt16(NXCPCodes.VID_OPERATION, 1);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}
	

	/**
	 * Unsubscribe from notification channel(s)
	 * 
	 * @param channels Notification channels to unsubscribe from. Multiple channels can be specified by combining them with OR operation.
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void unsubscribe(int channels) throws IOException, NXCException
	{
		NXCPMessage msg = newMessage(NXCPCodes.CMD_CHANGE_SUBSCRIPTION);
		msg.setVariableInt32(NXCPCodes.VID_FLAGS, channels);
		msg.setVariableInt16(NXCPCodes.VID_OPERATION, 0);
		sendMessage(msg);
		waitForRCC(msg.getMessageId());
	}
}
