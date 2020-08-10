/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
package org.netxms.websvc;

import org.netxms.client.TextOutputListener;

/**
 * Object tool output listener
 */
public class ObjectToolOutputListener implements TextOutputListener
{
   private Object mutex = new Object();
   private StringBuilder buffer = new StringBuilder();
   private boolean completed = false;
   private Long streamId = 0L;

   /**
    * @see org.netxms.client.TextOutputListener#messageReceived(java.lang.String)
    */
   @Override
   public void messageReceived(String text)
   {
      synchronized(mutex)
      {
         buffer.append(text);
         mutex.notifyAll();
      }
   }

   /**
    * @see org.netxms.client.TextOutputListener#setStreamId(long)
    */
   @Override
   public void setStreamId(long streamId)
   {
      this.streamId = streamId;
   }

   /**
    * @see org.netxms.client.TextOutputListener#onError()
    */
   @Override
   public void onError()
   {
   }

   /**
    * Get stream ID
    * 
    * @return stream ID
    */
   public long getStreamId()
   {
      return streamId;
   }

   /**
    * Read output of object tool. Read part of output is deleted from internal buffer.
    *
    * @return output text
    */
   public String readOutput() throws InterruptedException
   {
      String data;
      synchronized(mutex)
      {
         while((buffer.length() == 0) && !completed)
            mutex.wait();
         data = buffer.toString();
         buffer = new StringBuilder();
      }
      return data;
   }

   /**
    * Completion handler
    */
   public void onComplete()
   {
      synchronized(mutex)
      {
         completed = true;
         mutex.notifyAll();
      }
   }

   /**
    * Check if tool is completed
    * 
    * @return true if tool is completed
    */
   public boolean isCompleted()
   {
      return completed;
   }
}
