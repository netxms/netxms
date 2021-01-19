/**
 * 
 */
package org.netxms.reporting;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Startup class
 */
public final class Startup
{
   private static final Logger logger = LoggerFactory.getLogger(Startup.class);
   private static final Object shutdownLatch = new Object();

   /**
    * @param args
    */
   public static void main(String[] args)
   {
      Thread.currentThread().setName("Main Thread");

      Server server = new Server();
      try
      {
         server.init(null);
         server.start();
      }
      catch(Exception e)
      {
         logger.error("Server initialization failed", e);
         return;
      }

      Runtime.getRuntime().addShutdownHook(new Thread() {
         @Override
         public void run()
         {
            synchronized(shutdownLatch)
            {
               shutdownLatch.notify();
            }
         }
      });

      logger.info("Server instance initialized");
      while(true)
      {
         synchronized(shutdownLatch)
         {
            try
            {
               shutdownLatch.wait();
               break;
            }
            catch(InterruptedException e)
            {
               // ignore and wait again
            }
         }
      }

      try
      {
         server.stop();
         server.destroy();
      }
      catch(Exception e)
      {
         logger.error("Unexpected exception at shutdown", e);
      }

      logger.info("Server instance destroyed");
   }
}
