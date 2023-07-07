/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.reporting.extensions;

import org.netxms.reporting.Server;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Base class for report background worker. If defined, background worker will start when report is deployed, and stopped when
 * report is undeployed or reporting server is stopped.
 */
public abstract class AbstractBackgroundWorker
{
   private static final Logger logger = LoggerFactory.getLogger(AbstractBackgroundWorker.class);

   protected Server server;

   private Thread workerThread = null;
   private Object workerThreadMutex = new Object();
   private boolean stopped = false;

   /**
    * Start background worker.
    *
    * @param server server instance
    */
   public final synchronized void start(Server server)
   {
      if (workerThread != null)
      {
         logger.warn("Attempt to start already running background task");
         return;
      }
      this.server = server;
      workerThread = new Thread(new Runnable() {
         @Override
         public void run()
         {
            workerLoop();
         }
      }, "ReportBackgroundTask");
      stopped = false;
      workerThread.setDaemon(true);
      workerThread.start();
   }

   /**
    * Stop background worker
    */
   public final synchronized void stop()
   {
      if (workerThread == null)
      {
         logger.warn("Attempt to stop already stopped background task");
         return;
      }

      synchronized(workerThreadMutex)
      {
         stopped = true;
         workerThreadMutex.notifyAll();
      }

      try
      {
         workerThread.join();
      }
      catch(InterruptedException e)
      {
         logger.debug("Error joining worker thread", e);
      }
      workerThread = null;
   }

   /**
    * Worker main loop
    */
   private final void workerLoop()
   {
      int waitTime = 60;
      while(!stopped)
      {
         synchronized(workerThreadMutex)
         {
            try
            {
               workerThreadMutex.wait(waitTime * 1000);
            }
            catch(InterruptedException e)
            {
               break;
            }
            if (!stopped)
            {
               try
               {
                  waitTime = run();
               }
               catch(Exception e)
               {
                  logger.error("Unhandled exception in background worker", e);
               }
            }
         }
      }
   }

   /**
    * Execute background task once. Method should return numberof seconds until next execution.
    * 
    * @return number of seconds until next run
    */
   protected abstract int run();
}
