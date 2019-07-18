/**
 * 
 */
package org.netxms.nxmc.base.jobs;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;

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
   private int threadNumber = 1;
   private int jobId = 0;

   /**
    * Create job manager instance
    */
   private JobManager()
   {
      threadPool = Executors.newCachedThreadPool(new ThreadFactory() {
         @Override
         public Thread newThread(Runnable r)
         {
            return new Thread(r, "JobWorker-" + Integer.toString(threadNumber++));
         }
      });
   }

   /**
    * Submit job for execution
    *
    * @param job job object
    */
   protected synchronized void submit(final Job job)
   {
      job.setId(jobId++);
      jobs.put(job.getId(), job);
      threadPool.execute(new Runnable() {
         @Override
         public void run()
         {
            job.execute();
         }
      });
   }
}
