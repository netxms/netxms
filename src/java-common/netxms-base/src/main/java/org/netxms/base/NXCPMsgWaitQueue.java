/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.base;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * NXCP message wait queue
 */
public class NXCPMsgWaitQueue
{
	private List<NXCPMessage> messageList = new ArrayList<NXCPMessage>(0);
	private int defaultTimeout;
	private int messageLifeTime;
	private boolean isActive = true;
	private HousekeeperThread housekeeperThread = null;

   /**
	 * Housekeeper thread - deletes expired messages from queue.
	 */
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
					final Iterator<NXCPMessage> it = messageList.iterator();
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
    * Create message wait queue.
    * 
    * @param defaultTimeout default wait timeout in milliseconds
    * @param messageLifeTime message lifetime in milliseconds
    */
	public NXCPMsgWaitQueue(final int defaultTimeout, final int messageLifeTime)
	{
		this.defaultTimeout = defaultTimeout;
		this.messageLifeTime = messageLifeTime;
		housekeeperThread = new HousekeeperThread();
	}

	/**
    * Create message wait queue with default message lifetime (60 seconds).
    * 
    * @param defaultTimeout default wait timeout in milliseconds
    */
	public NXCPMsgWaitQueue(final int defaultTimeout)
	{
      this(defaultTimeout, 60000);
	}

	/**
	 * Put message into queue.
	 * 
	 * @param msg NXCP message
	 */
	public void putMessage(final NXCPMessage msg)
	{
		synchronized(messageList)
		{
			msg.setTimestamp(System.currentTimeMillis());
			messageList.add(msg);
			messageList.notifyAll();
		}
	}

	/**
	 * Wait for message.
	 * 
	 * @param code	Message code
	 * @param id Message id
	 * @param timeout Wait timeout in milliseconds
    * @return received NXCP message or null if message was not received before timeout
	 */
	public NXCPMessage waitForMessage(final int code, final long id, final int timeout)
	{
		NXCPMessage msg = null;
		int actualTimeout = timeout;

      while((actualTimeout > 0) && isActive)
		{
			synchronized(messageList)
			{
				final Iterator<NXCPMessage> it = messageList.iterator();
				boolean found = false;
				while(it.hasNext())
				{
					msg = it.next();
					if ((msg.getMessageCode() == code) && (msg.getMessageId() == id)) 
					{
						it.remove();
						found = true;
						break;
					}
				}
				if (found)
					break;
				
				msg = null;
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
    * Wait for message using default wait timeout.
    * 
    * @param code Message code
    * @param id Message id
    * @return received NXCP message or null if message was not received before timeout
    */
	public NXCPMessage waitForMessage(final int code, final long id)
	{
		return waitForMessage(code, id, defaultTimeout);
	}

	/**
	 * Shutdown wait queue.
	 */
   public synchronized void shutdown()
	{
		isActive = false;
		if (housekeeperThread != null)
		{
			housekeeperThread.interrupt();
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
      synchronized(messageList)
      {
         messageList.clear();
         messageList.notifyAll();
      }
	}

	/**
    * Get queue's default wait timeout
    * 
    * @return default wait timeout in milliseconds
    */
	public int getDefaultTimeout()
	{
		return defaultTimeout;
	}

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "NXCPMsgWaitQueue [messageList=" + messageList.toString()
            + " defaultTimeout=" + defaultTimeout + " messageLifeTime="
            + messageLifeTime + " isActive=" + isActive;
   }
}
