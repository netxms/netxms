/**
 * 
 */
package org.netxms.base;

import java.util.HashMap;
import java.util.Iterator;
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
	private HousekeeperThread housekeeperThread = null;
	
	
	//
	// Housekeeper thread
	//
	
	private class HousekeeperThread extends Thread
	{
		HousekeeperThread()
		{
			super("NXCPMsgWaitQueue::HousekeeperThread");
			setDaemon(true);
			start();
		}
		
		public void run()
		{
			while(isActive)
			{
				try
				{
					Thread.sleep(2000);
				}
				catch(InterruptedException e)
				{
				}
				synchronized(messageList)
				{
					final long currTime = System.currentTimeMillis();
					Iterator<NXCPMessage> it = messageList.values().iterator();
					while(it.hasNext())
					{
						final NXCPMessage msg = it.next();
						if (msg.getTimestamp() + messageLifeTime < currTime)
						{
							// Message expired, remove it
							it.remove();
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
	public NXCPMsgWaitQueue(final int defaultTimeout, final int messageLifeTime)
	{
		this.defaultTimeout = defaultTimeout;
		this.messageLifeTime = messageLifeTime;
		housekeeperThread = new HousekeeperThread();
	}

	/**
	 * @param defaultTimeout
	 */
	public NXCPMsgWaitQueue(final int defaultTimeout)
	{
		this.defaultTimeout = defaultTimeout;
		this.messageLifeTime = 60000;		// Default lifetime 60 seconds
		housekeeperThread = new HousekeeperThread();
	}
	
	
	//
	// Finalize
	//
	
	@Override
	protected void finalize()
	{
		shutdown();
	}
	
	
	//
	// Calculate hash for message
	//
	
	private Long calculateMessageHash(final NXCPMessage msg)
	{
		return (long)msg.getMessageCode() << 32 + msg.getMessageId();
	}

	
	//
	// Put message into queue
	//
	
	public void putMessage(final NXCPMessage msg)
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
	
	public NXCPMessage waitForMessage(final int code, final long id, final int timeout)
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

	public NXCPMessage waitForMessage(final int code, final long id)
	{
		return waitForMessage(code, id, defaultTimeout);
	}
	
	
	//
	// Shutdown wait queue
	//
	
	public void shutdown()
	{
		isActive = false;
		if (housekeeperThread != null)
		{
			while(housekeeperThread.isAlive())
			{
				try
				{
					housekeeperThread.join();
				}
				catch(InterruptedException e)
				{
				}
			}
			housekeeperThread = null;
		}
	}
}
