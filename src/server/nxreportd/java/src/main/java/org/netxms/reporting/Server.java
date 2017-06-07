/**
 * 
 */
package org.netxms.reporting;

/**
 * Server application class
 */
public final class Server
{
   private static Server instance = null;
   
   private ServerConnector connector;
   
   /**
    * Private constructor 
    */
   private Server() throws Exception
   {
      connector = new ServerConnector();
   }

   /**
    * Start server
    */
   public static boolean startServer()
   {
      if (instance != null)
         return false;
      
      try
      {
         instance = new Server();
         instance.start();
         return true;
      }
      catch(Exception e)
      {
         return false;
      }
   }
   
   public static void stopServer()
   {
      
   }
   
   private void start() throws Exception
   {
      connector.start();
   }
}
