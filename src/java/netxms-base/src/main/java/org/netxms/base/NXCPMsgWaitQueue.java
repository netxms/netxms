/**
 * 
 */
package org.netxms.base;

import java.util.HashMap;
import java.util.Map;


/**
 * @author victor
 *
 */

public class NXCPMsgWaitQueue
{
	private Map<Long, NXCPMessage> messageList = new HashMap<Long, NXCPMessage>(0);
	private int defaultTimeout;
	private int messageLifeTime;
	private boolean isActive = true;
	
	
	//
	// Housekeeper thread
	//
	
	private class HousekeeperThread extends Thread
	{
		HousekeeperThread()
		{
			setDaemon(true);
			start();
		}
		
		public void run()
		{
			while(isActive)
			{
				try
				{
					Thread.sleep(10000);
				}
				catch(InterruptedException e)
				{
				}
				synchronized(messageList)
				{
					long currTime = System.currentTimeMillis();
					for(NXCPMessage msg: messageList.values())
					{
						if (msg.getTimestamp() + messageLifeTime < currTime)
						{
							// Message expired, remove it
							messageList.remove(calculateMessageHash(msg));
						}
					}
				}
			}
		}
	}
	
	
	/**
	 * @param defaultTimeout
	 * @param messageLifeTime
	 */
	public NXCPMsgWaitQueue(int defaultTimeout, int messageLifeTime)
	{
		this.defaultTimeout = defaultTimeout;
		this.messageLifeTime = messageLifeTime;
		new HousekeeperThread();
	}

	/**
	 * @param defaultTimeout
	 */
	public NXCPMsgWaitQueue(int defaultTimeout)
	{
		this.defaultTimeout = defaultTimeout;
		this.messageLifeTime = 60000;		// Default lifetime 60 seconds
		new HousekeeperThread();
	}
	
	
	//
	// Calculate hash for message
	//
	
	private Long calculateMessageHash(NXCPMessage msg)
	{
		return (long)msg.getMessageCode() << 32 + msg.getMessageId();
	}

	
	//
	// Put message into queue
	//
	
	public void putMessage(NXCPMessage msg)
	{
		synchronized(messageList)
		{
			msg.setTimestamp(System.currentTimeMillis());
			messageList.put(calculateMessageHash(msg), msg);
			messageList.notifyAll();
		}
	}

	
	/**
	 * @param code	Message code
	 * @param id Message id
	 * @param timeout Wait timeout in milliseconds
	 */
	
	public NXCPMessage waitForMessage(int code, long id, int timeout)
	{
		final Long hash = (long)code << 32 + id;
		NXCPMessage msg = null;
		int actualTimeout = timeout;
		
		while(actualTimeout > 0)
		{
			synchronized(messageList)
			{
				msg = messageList.get(hash);
				if (msg != null)
					break;
				
				long startTime = System.currentTimeMillis();
				try
				{
					messageList.wait(actualTimeout);
				}
				catch(InterruptedException e)
				{
				}
				actualTimeout -= System.currentTimeMillis() - startTime;
			}
		}
		
		return msg;
	}


	/**
	 * @param code	Message code
	 * @param id Message id
	 */

	public NXCPMessage waitForMessage(int code, long id)
	{
		return waitForMessage(code, id, defaultTimeout);
	}
	
	
	//
	// Shutdown wait queue
	//
	
	public void shutdown()
	{
		isActive = false;
	}
}
