/**
 * 
 */
package org.netxms.reporting;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;

/**
 * Connector
 */
public class ServerConnector
{
   private static final int PORT = 4710;

   private ServerSocket serverSocket;
   private Thread listenerThread;
   private List<Session> sessions;
   
   /**
    * 
    */
   public ServerConnector() throws IOException
   {
      serverSocket = new ServerSocket(PORT);
      sessions = new ArrayList<Session>(4);
   }

   /**
    * Run connector
    */
   public void start() throws Exception
   {
      listenerThread = new Thread(new Runnable() {
         @Override
         public void run()
         {
            while(!Thread.currentThread().isInterrupted())
            {
               try
               {
                  final Socket socket = serverSocket.accept();
                  Session session = new Session(socket);
                  sessions.add(session);
                  session.start();
               }
               catch(IOException e)
               {
                  e.printStackTrace();
               }
            }
         }
      });
      listenerThread.start();
   }

   /**
    * Stop connector
    */
   public void stop() 
   {
      listenerThread.interrupt();
      try 
      {
         listenerThread.join();
      } 
      catch(InterruptedException e) 
      {
      }
      
      for(Session s : sessions)
         s.shutdown();
  }
}
