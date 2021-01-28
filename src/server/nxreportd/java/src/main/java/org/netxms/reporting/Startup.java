/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
    * Entry point
    *
    * @param args command line arguments
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

   /**
    * Stop server (intended for call by prunsrv.exe)
    *
    * @param args command line arguments
    */
   public static void stop(String[] args)
   {
      synchronized(shutdownLatch)
      {
         shutdownLatch.notify();
      }
   }
}
