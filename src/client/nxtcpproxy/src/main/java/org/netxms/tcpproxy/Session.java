/**
 * 
 */
package org.netxms.tcpproxy;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import org.netxms.client.TcpProxy;

/**
 * Proxy session
 */
public class Session
{
   private int id;
   private Socket socket;
   private TcpProxy proxy;
   private Thread socketReaderThread;
   private Thread proxyReaderThread;
   
   public Session(int id, Socket socket, TcpProxy proxy)
   {
      this.id = id;
      this.socket = socket;
      this.proxy = proxy;
      
      socketReaderThread = new Thread(new Runnable() {
         @Override
         public void run()
         {
            print("Socket reader started");
            socketReader();
         }
      });
      proxyReaderThread = new Thread(new Runnable() {
         @Override
         public void run()
         {
            print("Proxy reader started");
            proxyReader();
         }
      });
      
      socketReaderThread.start();
      proxyReaderThread.start();
   }
   
   private void socketReader()
   {
      try
      {
         InputStream in = socket.getInputStream();
         byte[] buffer = new byte[32768];
         while(socket.isConnected())
         {
            int bytes = in.read(buffer);
            if (bytes <= 0)
               break;
            proxy.getOutputStream().write(buffer, 0, bytes);
         }
      }
      catch(Exception e)
      {
         e.printStackTrace();
      }

      proxy.close();
      
      print("Waiting for proxy reader to stop");
      try
      {
         proxyReaderThread.join();
      }
      catch(InterruptedException e)
      {
         e.printStackTrace();
      }
      
      try
      {
         socket.close();
      }
      catch(IOException e)
      {
      }
      print("Socket reader terminated");
      
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
         while(!proxy.isClosed())
         {
            int bytes = in.read(buffer);
            if (bytes <= 0)
               break;
            out.write(buffer, 0, bytes);
         }
      }
      catch(Exception e)
      {
         e.printStackTrace();
      }

      try
      {
         socket.close();
      }
      catch(IOException e)
      {
      }
      print("Proxy reader terminated");
   }

   private void print(String text)
   {
      System.out.println("<" + id + "> " + text);
   }
}
