/**
 * 
 */
package org.netxms.tcpproxy;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import org.netxms.client.TcpProxy;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Proxy session
 */
public class Session
{
   private static final Logger logger = LoggerFactory.getLogger(Session.class);

   private Socket socket;
   private TcpProxy proxy;
   private Thread socketReaderThread;
   private Thread proxyReaderThread;
   
   public Session(int id, Socket socket, TcpProxy proxy)
   {
      this.socket = socket;
      this.proxy = proxy;
      
      socketReaderThread = new Thread(new Runnable() {
         @Override
         public void run()
         {
            logger.info("Socket reader started");
            socketReader();
         }
      }, "Session-" + id + "-Socket");
      proxyReaderThread = new Thread(new Runnable() {
         @Override
         public void run()
         {
            logger.info("Proxy reader started");
            proxyReader();
         }
      }, "Session-" + id + "-Proxy");

      socketReaderThread.start();
      proxyReaderThread.start();
   }

   private void socketReader()
   {
      try
      {
         InputStream in = socket.getInputStream();
         byte[] buffer = new byte[32768];
         while(true)
         {
            int bytes = in.read(buffer);
            if (bytes <= 0)
            {
               logger.info("Exit code " + bytes + " while reading socket input stream");
               break;
            }
            proxy.getOutputStream().write(buffer, 0, bytes);
         }
      }
      catch(Exception e)
      {
         logger.error("Socket reader exception", e);
      }

      proxy.close();
      
      logger.info("Waiting for proxy reader to stop");
      try
      {
         proxyReaderThread.join();
      }
      catch(InterruptedException e)
      {
         logger.error("Thread join exception", e);
      }

      try
      {
         socket.close();
      }
      catch(IOException e)
      {
      }
      logger.info("Socket reader terminated");
      
      socket = null;
      proxy = null;
   }
   
   private void proxyReader()
   {
      try
      {
         InputStream in = proxy.getInputStream();
         OutputStream out = socket.getOutputStream();
         byte[] buffer = new byte[32768];
         while(true)
         {
            int bytes = in.read(buffer);
            if (bytes <= 0)
            {
               logger.info("Exit code " + bytes + " while reading proxy input stream");
               break;
            }
            out.write(buffer, 0, bytes);
         }
      }
      catch(Exception e)
      {
         logger.error("Proxy reader exception", e);
      }

      logger.info("Proxy reader requesting local socket closure");
      try
      {
         socket.close();
      }
      catch(IOException e)
      {
      }
      logger.info("Proxy reader terminated");
   }
}
