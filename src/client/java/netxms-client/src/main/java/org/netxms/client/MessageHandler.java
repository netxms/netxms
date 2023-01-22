/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.client;

import java.util.concurrent.CountDownLatch;
import org.netxms.base.NXCPMessage;

/**
 * Message handler interface
 */
public abstract class MessageHandler
{
   private CountDownLatch latch = new CountDownLatch(1);
   private boolean expired = false;
   private long lastMessageTimestamp = System.currentTimeMillis();
   private int messageWaitTimeout = 60000;

   /**
    * Set handler to complete state. This will signal all waiters and remove subscription. 
    */
   protected final void setComplete()
   {
      latch.countDown();
   }

   /**
    * Get completion flag
    * 
    * @return returns if message is complete
    */
   public final boolean isComplete()
   {
      return latch.getCount() == 0;
   }

   /**
    * Wait for handler completion
    */
   public final void waitForCompletion()
   {
      boolean done = false;
      do
      {
         try
         {
            latch.await();
            done = true;
         }
         catch(InterruptedException e)
         {
         }
      }
      while(!done);
   }

   /**
    * @return the lastMessageTimestamp
    */
   protected long getLastMessageTimestamp()
   {
      return lastMessageTimestamp;
   }

   /**
    * @param lastMessageTimestamp the lastMessageTimestamp to set
    */
   protected void setLastMessageTimestamp(long lastMessageTimestamp)
   {
      this.lastMessageTimestamp = lastMessageTimestamp;
   }

   /**
    * Check if handler is expired.
    *
    * @return true if handler is expired
    */
   public boolean isExpired()
   {
      return expired;
   }

   /**
    * Set handler as expired (also sets completion flag)
    */
   protected void setExpired()
   {
      expired = true;
      setComplete();
   }

   /**
    * @return the messageWaitTimeout
    */
   public int getMessageWaitTimeout()
   {
      return messageWaitTimeout;
   }

   /**
    * @param messageWaitTimeout the messageWaitTimeout to set
    */
   public void setMessageWaitTimeout(int messageWaitTimeout)
   {
      this.messageWaitTimeout = messageWaitTimeout;
   }

   /**
    * Process message. If handler returns true message will not be placed into waiting queue.
    * 
    * @param msg NXCP message to process
    * @return true if message is processed
    */
   abstract public boolean processMessage(NXCPMessage msg);
}
