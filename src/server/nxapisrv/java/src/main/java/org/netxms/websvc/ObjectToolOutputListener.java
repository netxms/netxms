package org.netxms.websvc;

import org.netxms.client.TextOutputListener;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ObjectToolOutputListener implements TextOutputListener
{
   private Logger log = LoggerFactory.getLogger(ObjectToolOutputListener.class);
   private Object mutex = new Object();
   private StringBuilder buffer = new StringBuilder();
   private boolean completed = false;
   private Long streamId = 0L;

   @Override
   public void messageReceived(String text)
   {
      synchronized(mutex)
      {
         buffer.append(text);
         mutex.notifyAll();
      }
   }

   @Override
   public void setStreamId(long streamId)
   {
      this.streamId = streamId;
   }

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

   public void onComplete()
   {
      synchronized(mutex)
      {
         completed = true;
         mutex.notifyAll();
      }
   }

   public boolean isCompleted()
   {
      return completed;
   }
}
