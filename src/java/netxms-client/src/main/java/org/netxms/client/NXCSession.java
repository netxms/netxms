/**
 *
 */
package org.netxms.client;

import java.io.*;
import java.net.*;
import org.netxms.base.*;

/**
 * @author victor
 */
public class NXCSession
{
	// Public constants
	public static final int DEFAULT_CONN_PORT = 4701;
	
	// Connection-related attributes
	private String connAddress;
	private int connPort;
	private String connLoginName;
	private String connPassword;
	private boolean connUseEncryption;
	private Socket connSocket;
	private NXCPMsgWaitQueue msgWaitQueue;
	private boolean isActive = true;
	
	
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
			while(isActive)
			{
				
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
	// Connect to server
	//
	
	public void connect() throws IOException, UnknownHostException
	{
		connSocket = new Socket(connAddress, connPort);
		msgWaitQueue = new NXCPMsgWaitQueue(10000);
		new ReceiverThread();
	}
	
	
	//
	// Disconnect
	//
	
	public void disconnect()
	{
		isActive = false;
	}
}
