/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import java.lang.reflect.InvocationTargetException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.swt.widgets.Display;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Background job.
 */
public abstract class Job
{
   private static final Logger logger = LoggerFactory.getLogger(Job.class);

   private static final int TYPE_SYSTEM = 0;
   private static final int TYPE_DEFAULT = 1;
   private static final int TYPE_USER = 2;

   private int id;
   private JobState state;
   private Display display;
   private View view;
   private MessageAreaHolder messageArea;
   private String name;
   private int type;
   private IProgressMonitor monitor;

   /**
    * Constructor for console job object
    * 
    * @param name Job's name
    * @param view Related view or null
    */
   public Job(String name, View view)
   {
      this(name, view, view, Display.getCurrent());
   }

   /**
    * Constructor for console job object
    * 
    * @param name Job's name
    * @param view Related view or null
    * @param messageArea Dedicated message area for displaying error or null
    */
   public Job(String name, View view, MessageAreaHolder messageArea)
   {
      this(name, view, (messageArea != null) ? messageArea : view, Display.getCurrent());
   }

   /**
    * Constructor for console job object
    * 
    * @param name Job's name
    * @param view Related view or null
    * @param messageArea Dedicated message area for displaying error or null
    * @param display display object used for execution in UI thread
    */
   public Job(String name, View view, MessageAreaHolder messageArea, Display display)
   {
      this.id = -1;
      this.state = JobState.PENDING;
      this.name = name;
      this.type = TYPE_USER;
      this.view = view;
      this.messageArea = (messageArea != null) ? messageArea : view;
      this.display = display;
   }

   /**
    * Set job ID (intended to be called only by job manager).
    * 
    * @param id new job ID
    */
   protected void setId(int id)
   {
      this.id = id;
   }

   /**
    * Get job ID.
    * 
    * @return job ID
    */
   public int getId()
   {
      return id;
   }

   /**
    * Get job state.
    *
    * @return job state
    */
   public JobState getState()
   {
      return state;
   }

   /**
    * Execute job.
    * 
    * @param monitor progress monitor (can be null)
    */
   protected final void execute(IProgressMonitor monitor)
   {
      if (monitor == null)
         monitor = new JobProgressMonitor();
      this.monitor = monitor;

      state = JobState.RUNNING;

      try
      {
         monitor.beginTask(name, IProgressMonitor.UNKNOWN);
         run(monitor);
      }
      catch(Exception e)
      {
         logger.error("Exception in UI job - " + e.getMessage(), e);
         jobFailureHandler(e);
         if (messageArea != null)
         {
            runInUIThread(() -> messageArea.addMessage(MessageArea.ERROR, getErrorMessage() + ": " + e.getLocalizedMessage()));
         }
      }
      finally
      {
         monitor.done();
         jobFinalize();
         state = JobState.COMPLETED;
         if (view != null)
         {
            runInUIThread(() -> view.onJobCompletion(id));
         }
      }
   }

   /**
    * Start job.
    */
   public void start()
   {
      JobManager.getInstance().submit(this, null);
   }

   /**
    * Start job using provided progress monitor.
    * 
    * @param monitor progress monitor
    */
   public void start(IProgressMonitor monitor)
   {
      JobManager.getInstance().submit(this, monitor);
   }

   /**
    * Schedule job for later execution.
    *
    * @param delay execution delay in milliseconds
    */
   public void schedule(long delay)
   {
      JobManager.getInstance().schedule(this, delay);
   }

   /**
    * Run job in foreground
    */
   public void runInForeground()
   {
      ProgressMonitorDialog monitorDialog = new ProgressMonitorDialog(null);
      try
      {
         monitorDialog.run(true, false, new IRunnableWithProgress() {
            @Override
            public void run(IProgressMonitor monitor) throws InvocationTargetException, InterruptedException
            {
               state = JobState.RUNNING;
               try
               {
                  Job.this.run(monitor);
               }
               catch(Exception e)
               {
                  throw new InvocationTargetException(e);
               }
               finally
               {
                  jobFinalize();
                  state = JobState.COMPLETED;
               }
            }
         });
      }
      catch(Exception e)
      {
         logger.error("Exception in foreground UI job - " + e.getMessage(), e);
      }
   }

   /**
    * Set/remove system background job mark.
    * 
    * @param user true to mark job as system background job
    */
   public void setSystem(boolean system)
   {
      if (system)
         type = TYPE_SYSTEM;
      else if (type == TYPE_SYSTEM)
         type = TYPE_DEFAULT;
   }

   /**
    * Check if job is marked as background system task.
    * 
    * @return true if job is marked as background system task
    */
   public boolean isSystem()
   {
      return type == TYPE_SYSTEM;
   }

   /**
    * Set/remove user initiated mark.
    * 
    * @param user true to mark job as user-initiated
    */
   public void setUser(boolean user)
   {
      if (user)
         type = TYPE_USER;
      else if (type == TYPE_USER)
         type = TYPE_DEFAULT;
   }

   /**
    * Check if job is marked as user-initiated.
    * 
    * @return true if job is marked as user-initiated
    */
   public boolean isUser()
   {
      return type == TYPE_USER;
   }

   /**
    * Get job's name.
    * 
    * @return job's name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get job's view.
    * 
    * @return job's view or null if not associated with any view
    */
   public View getView()
   {
      return view;
   }

   /**
    * Executes job. Called from within Job.execute(). If job fails, this method should throw appropriate exception.
    * 
    * @param monitor the monitor to be used for reporting progress and responding to cancellation. The monitor is never null.
    * @throws Exception in case of any failure.
    */
   protected abstract void run(IProgressMonitor monitor) throws Exception;

   /**
    * Should return error message which will be shown in case of job failure. Result of exception's getMessage() will be appended to
    * this message.
    * 
    * @return Error message
    */
   protected abstract String getErrorMessage();

   /**
    * Called from within Job.run() if job has failed. Default implementation does nothing.
    * 
    * @param e exception that cause job to abort
    */
   protected void jobFailureHandler(Exception e)
   {
   }

   /**
    * Called from within Job.run() if job completes, either successfully or not. Default implementation does nothing.
    */
   protected void jobFinalize()
   {
   }

   /**
    * Run code in UI thread
    * 
    * @param runnable
    */
   protected void runInUIThread(final Runnable runnable)
   {
      if (!display.isDisposed())
         display.asyncExec(runnable);
   }

   /**
    * @return
    */
   protected Display getDisplay()
   {
      return display;
   }

   /**
    * Set job to "scheduled" state
    */
   protected void setScheduledState()
   {
      state = JobState.SCHEDULED;
   }

   /**
    * Cancel this job. Will have no effect if job is not in running state.
    */
   public void cancel() 
   {
      if (state == JobState.RUNNING)
      {
         canceling();
         monitor.setCanceled(true);
      }
   }

   /**
    * A hook method indicating that this job is running and {@link #cancel()}
    * is being called for the first time.
    * <p>
    * Subclasses may override this method to perform additional work when
    * a cancelation request is made.  This default implementation does nothing.
    */
   protected void canceling() 
   {
      //default implementation does nothing
   }

   /**
    * Internal progress monitor
    */
   private class JobProgressMonitor implements IProgressMonitor
   {
      @Override
      public void beginTask(String name, int totalWork)
      {
      }

      @Override
      public void done()
      {
      }

      @Override
      public void internalWorked(double work)
      {
      }

      @Override
      public boolean isCanceled()
      {
         return false;
      }

      @Override
      public void setCanceled(boolean value)
      {
      }

      @Override
      public void setTaskName(String name)
      {
      }

      @Override
      public void subTask(String name)
      {
      }

      @Override
      public void worked(int work)
      {
      }
   }
}
