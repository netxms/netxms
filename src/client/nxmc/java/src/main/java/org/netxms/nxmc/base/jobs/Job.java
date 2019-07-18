/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.widgets.Display;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MessageBar;
import org.slf4j.LoggerFactory;

/**
 * Background job.
 */
public abstract class Job
{
   private static final int TYPE_SYSTEM = 0;
   private static final int TYPE_DEFAULT = 1;
   private static final int TYPE_USER = 2;

   private int id;
   private Display display;
   private View view;
   private MessageBar messageBar;
   private String name;
   private int type;
   private IProgressMonitor monitor;

   /**
    * Constructor for console job object
    * 
    * @param name Job's name
    * @param viewPart Related view part or null
    * @param pluginId Related plugin ID
    */
   public Job(String name, View view)
   {
      this(name, view, null, Display.getCurrent());
   }

   /**
    * Constructor for console job object
    * 
    * @param name Job's name
    * @param viewPart Related view part or null
    * @param pluginId Related plugin ID
    * @param messageBar Message bar for displaying error or null
    */
   public Job(String name, View view, MessageBar messageBar)
   {
      this(name, view, messageBar, Display.getCurrent());
   }

   /**
    * Constructor for console job object
    * 
    * @param name Job's name
    * @param viewPart Related view part or null
    * @param pluginId Related plugin ID
    * @param messageBar Message bar for displaying error or null
    * @param display display object used for execution in UI thread
    */
   public Job(String name, View view, MessageBar messageBar, Display display)
   {
      this.id = -1;
      this.name = name;
      this.type = TYPE_USER;
      this.view = view;
      this.messageBar = messageBar;
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
    * Execute job.
    */
   protected void execute()
   {
      monitor = new JobProgressMonitor();
      try
      {
         run(monitor);
         if (messageBar != null)
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  messageBar.hideMessage();
               }
            });
         }
      }
      catch(Exception e)
      {
         LoggerFactory.getLogger(Job.class).error("Exception in UI job - " + e.getMessage(), e);
         jobFailureHandler();
         if (messageBar != null)
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  messageBar.showMessage(MessageBar.ERROR, getErrorMessage() + ": " + e.getMessage());
               }
            });
         }
      }
      finally
      {
         monitor.done();
         jobFinalize();
      }
   }

   /**
    * Start job.
    */
   public void start()
   {
      JobManager.getInstance().submit(this);
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
    */
   protected void jobFailureHandler()
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
