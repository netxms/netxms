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
package org.netxms.nxmc.modules.objects;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import org.netxms.client.TcpProxy;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * TCP port forwarder
 */
public class TcpPortForwarder
{
   private static final Logger logger = LoggerFactory.getLogger(TcpPortForwarder.class);

   private TcpProxy proxy;
   private ServerSocket listener;
   private int sessionId = 0;

   /**
    * Create new port forwarder instance. Became owner of TCP proxy and will call <code>TCPProxy.close()</code> as needed (including
    * listener setup error).
    *
    * @param proxy underlying TCP proxy object
    * @throws IOException if cannot setup local TCP port listener
    */
   public TcpPortForwarder(TcpProxy proxy) throws IOException
   {
      this.proxy = proxy;
      try
      {
         listener = new ServerSocket(0);
      }
      catch(IOException e)
      {
         proxy.close();
         throw e;
      }
   }

   /**
    * Run port forwarder (will start background thread accepting connection on local port).
    *
    * @throws Exception on any error
    */
   public void run() throws Exception
   {
      final Object mutex = new Object();
      Thread thread = new Thread(new Runnable() {
         @Override
         public void run()
         {
            logger.info("TCP port forwarder listening on port " + listener.getLocalPort());
            synchronized(mutex)
            {
               mutex.notifyAll();
            }
            try
            {
               while(true)
               {
                  final Socket socket = listener.accept();
                  try
                  {
                     new Session(++sessionId, socket);
                  }
                  catch(Exception e)
                  {
                     socket.close();
                     logger.error("TCP port forwarder session setup error", e);
                  }
               }
            }
            catch(Exception e)
            {
               logger.error("TCP port forwarder listener loop error", e);
            }
            finally
            {
               proxy.close();
               try
               {
                  listener.close();
               }
               catch(IOException e)
               {
               }
            }
         }
      }, "TcpForwarder");
      thread.setDaemon(true);
      synchronized(mutex)
      {
         thread.start();
         mutex.wait(); // wait for listener thread start
         Thread.sleep(100); // Additional wait to ensure that accept() is called on listening socket
      }
   }

   /**
    * Close port forwarder. Will also close underlying TCP proxy object.
    */
   public void close()
   {
      logger.debug("Closing TCP forwarder instance on port " + listener.getLocalPort());
      try
      {
         listener.close();
      }
      catch(Exception e)
      {
         logger.debug("Error closing listening socket", e);
      }
   }

   /**
    * Get local port number.
    *
    * @return local port number
    */
   public int getLocalPort()
   {
      return listener.getLocalPort();
   }

   /**
    * Port forwarding session
    */
   private class Session
   {
      private Socket socket;
      private Thread socketReaderThread;
      private Thread proxyReaderThread;

      public Session(int id, Socket socket)
      {
         this.socket = socket;

         socketReaderThread = new Thread(new Runnable() {
            @Override
            public void run()
            {
               logger.info("Socket reader started");
               socketReader();
            }
         }, "Session-" + id + "-Socket");
         socketReaderThread.setDaemon(true);

         proxyReaderThread = new Thread(new Runnable() {
            @Override
            public void run()
            {
               logger.info("Proxy reader started");
               proxyReader();
            }
         }, "Session-" + id + "-Proxy");
         proxyReaderThread.setDaemon(true);

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
}
