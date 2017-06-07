/**
 * 
 */
package org.netxms.reporting;

import java.io.FileInputStream;
import java.io.IOException;
import java.net.Socket;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPException;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCPMessageReceiver;
import org.python.modules.synchronize;
import com.radensolutions.reporting.service.MessageProcessingResult;

/**
 * Server session
 */
public class Session
{
   private Socket socket;
   private Thread receiverThread;
   
   /**
    * 
    */
   public Session(Socket socket)
   {
      this.socket = socket;
      receiverThread = null;
   }

   /**
    * Start session
    */
   public synchronized void start()
   {
      receiverThread = new Thread(new Runnable() {
         @Override
         public void run()
         {
            receiverThread();
         }
      }, "Session receiver thread");
      receiverThread.run();
   }
   
   /**
    * Shutdown session
    */
   public synchronized void shutdown()
   {
      if (receiverThread != null)
      {
         try
         {
            receiverThread.join();
         }
         catch(InterruptedException e)
         {
            // TODO Auto-generated catch block
            e.printStackTrace();
         }
      }
   }
   
   private void receiverThread()
   {
      final NXCPMessageReceiver messageReceiver = new NXCPMessageReceiver(262144, 4194304);   // 256KB, 4MB
      while(!Thread.currentThread().isInterrupted())
      {
         try
         {
            final NXCPMessage message = messageReceiver.receiveMessage(socket.getInputStream(), null);
            if (message != null)
            {
               if (message.getMessageCode() != NXCPCodes.CMD_KEEPALIVE)
               {
                  log.debug("Request: " + message.toString());
               }
               final MessageProcessingResult result = session.processMessage(message);
               if (result.response != null)
               {
                  if (message.getMessageCode() != NXCPCodes.CMD_KEEPALIVE)
                  {
                     log.debug("Reply: " + result.response.toString());
                  }
                  sendMessage(result.response);
                  if (result.file != null)
                  {
                     log.debug("File data found, sending");
                     FileInputStream s = null;
                     try
                     {
                        s = new FileInputStream(result.file);
                        sendFileStream(message.getMessageId(), s);
                     }
                     catch(IOException e)
                     {
                        log.error("Unexpected I/O exception while sending rendered file");
                     }
                     if (s != null)
                        s.close();
                     result.file.delete();
                  }
               }
            }
         }
         catch(IOException e)
         {
            log.info("Communication error", e);
            stop();
         }
         catch(NXCPException e)
         {
            log.info("Invalid message received", e);
            stop();
         }
      }
   }
}
