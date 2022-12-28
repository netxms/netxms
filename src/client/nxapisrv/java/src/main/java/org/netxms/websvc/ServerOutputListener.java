/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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

import java.util.Date;
import org.netxms.client.NXCException;
import org.netxms.client.TextOutputListener;

/**
 * Object tool output listener
 */
public class ServerOutputListener implements TextOutputListener
{
   private Object mutex = new Object();
   private StringBuilder buffer = new StringBuilder();
   private boolean completed = false;
   private Long streamId = 0L;
   private Date lastUpdateTime;

   /**
    * @see org.netxms.client.TextOutputListener#messageReceived(java.lang.String)
    */
   @Override
   public void messageReceived(String text)
   {
      lastUpdateTime = new Date();
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
    * @see org.netxms.client.TextOutputListener#onSuccess()
    */
   @Override
   public void onSuccess()
   {
      lastUpdateTime = new Date();
      synchronized(mutex)
      {
         completed = true;
         mutex.notifyAll();
      }
   }

   /**
    * @see org.netxms.client.TextOutputListener#onFailure(java.lang.Exception)
    */
   @Override
   public void onFailure(Exception exception)
   {
      lastUpdateTime = new Date();
      synchronized(mutex)
      {
         buffer.append((exception instanceof NXCException) ? exception.getMessage() : String.format("Internal error (%s)", exception.getClass().getName()));
         completed = true;
         mutex.notifyAll();
      }
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
      lastUpdateTime = new Date();
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
    * Check if tool is completed
    * 
    * @return true if tool is completed
    */
   public boolean isCompleted()
   {
      return completed;
   }

   /**
    * Check if tool is completed
    * 
    * @return true if tool is completed
    */
   public Date getLastUpdateTime()
   {
      return lastUpdateTime;
   }
}
