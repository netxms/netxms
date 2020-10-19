/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Raden Soultions
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
package org.netxms.ui.eclipse.objecttools.widgets;

import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.console.IOConsoleOutputStream;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objects.ObjectContext;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;

/**
 * Local command executer and result provider widget
 */
public class LocalCommandExecutor extends AbstractObjectToolExecutor
{
   private Process process;
   private boolean processRunning = false;
   private String command;
   private Object mutex = new Object();

   /**
    * Constructor 
    * 
    * @param result Area area to display result
    * @param view parent view
    * @param command command to execute
    */
   public LocalCommandExecutor(Composite resultArea, ViewPart view, ObjectContext objectContext, ActionSet actionSet, String command)
   {
      super(resultArea, view, objectContext, actionSet);
      this.command = command;
   }
   
   /**
    * @see org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor#execute()
    */
   @Override
   public void execute()
   {
      synchronized(mutex)
      {
         if (processRunning)
         {
            process.destroy();
            try
            {
               mutex.wait();
            }
            catch(InterruptedException e)
            {
            }
         }
         processRunning = true;
         setRunning(true);
      }
      
      final IOConsoleOutputStream out = console.newOutputStream();
      ConsoleJob job = new ConsoleJob(Messages.get().LocalCommandResults_JobTitle, viewPart, Activator.PLUGIN_ID, null) {
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().LocalCommandResults_JobError;
         }

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            process = Runtime.getRuntime().exec(command);
            InputStream in = process.getInputStream();
            try
            {
               byte[] data = new byte[16384];
               boolean isWindows = Platform.getOS().equals(Platform.OS_WIN32);
               while(true)
               {
                  int bytes = in.read(data);
                  if (bytes == -1)
                     break;
                  String s = new String(Arrays.copyOf(data, bytes));
                  
                  // The following is a workaround for issue NX-65
                  // Problem is that on Windows XP many system commands
                  // (like ping, tracert, etc.) generates output with lines
                  // ending in 0x0D 0x0D 0x0A
                  if (isWindows)
                     out.write(s.replace("\r\r\n", " \r\n")); //$NON-NLS-1$ //$NON-NLS-2$
                  else
                     out.write(s);
               }
               
               out.write(Messages.get().LocalCommandResults_Terminated);
            }
            catch(IOException e)
            {
               Activator.logError("Exception while running local command", e); //$NON-NLS-1$
            }
            finally
            {
               in.close();
               out.close();
            }
         }

         @Override
         protected void jobFinalize()
         {
            synchronized(mutex)
            {
               processRunning = false;
               process = null;
               mutex.notifyAll();
            }

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  synchronized(mutex)
                  {
                     setRunning(processRunning);
                  }
               }
            });
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();

   }

   /**
    * @see org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor#terminate()
    */
   @Override
   public void terminate()
   {
      synchronized(mutex)
      {
         if (processRunning)
         {
            process.destroy();
         }
      }
   }

   /**
    * @see org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor#isTerminateSupported()
    */
   @Override
   protected boolean isTerminateSupported()
   {
      return true;
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      synchronized(mutex)
      {
         if (processRunning)
         {
            process.destroy();
         }
      }
      super.dispose();
   }
}
