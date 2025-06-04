/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.base.jobs;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;
import org.eclipse.core.runtime.IProgressMonitor;

/**
 * Job manager
 */
public class JobManager
{
   private static JobManager instance = new JobManager();

   /**
    * Get instance of job manager
    * 
    * @return instance of job manager
    */
   public static JobManager getInstance()
   {
      return instance;
   }

   private Map<Integer, Job> jobs = new HashMap<Integer, Job>();
   private ExecutorService threadPool;
   private ScheduledExecutorService scheduler;
   private int jobId = 0;

   /**
    * Create job manager instance
    */
   private JobManager()
   {
      threadPool = Executors.newCachedThreadPool(new ThreadFactory() {
         private int threadNumber = 1;

         @Override
         public Thread newThread(Runnable r)
         {
            Thread t = new Thread(r, "JobWorker-" + Integer.toString(threadNumber++));
            t.setDaemon(true);
            return t;
         }
      });
      scheduler = Executors.newSingleThreadScheduledExecutor(new ThreadFactory() {
         @Override
         public Thread newThread(Runnable r)
         {
            Thread t = new Thread(r, "JobScheduler");
            t.setDaemon(true);
            return t;
         }
      });
   }

   /**
    * Submit job for execution
    *
    * @param job job object
    * @param monitor optional progress monitor
    */
   protected synchronized void submit(final Job job, IProgressMonitor monitor)
   {
      // Call submit in UI thread to avoid deadlock
      if ((job.getDisplay().getThread() != Thread.currentThread()) && (monitor == null) && (job.getView() != null))
      {
         job.getDisplay().asyncExec(() -> {
            submit(job, monitor);
         });
         return;
      }

      job.setId(jobId++);
      jobs.put(job.getId(), job);

      final IProgressMonitor finalMonitor;
      if ((monitor == null) && (job.getView() != null) && job.isUser())
      {
         finalMonitor = job.getView().createProgressMonitor(job.getId());
      }
      else
      {
         finalMonitor = monitor;
      }

      threadPool.execute(() -> {
         job.execute(finalMonitor);
         onJobCompletion(job);
      });
   }

   /**
    * Schedule job for later execution.
    *
    * @param job job to execute
    * @param delay execution delay in milliseconds
    */
   protected synchronized void schedule(final Job job, long delay)
   {
      job.setId(jobId++);
      job.setScheduledState();
      jobs.put(job.getId(), job);
      scheduler.schedule(() -> {
         threadPool.execute(() -> {
            job.execute(null);
            onJobCompletion(job);
         });
      }, delay, TimeUnit.MILLISECONDS);
   }

   /**
    * Process job completion.
    *
    * @param job completed job
    */
   private synchronized void onJobCompletion(Job job)
   {
      jobs.remove(job.getId());
   }
}
