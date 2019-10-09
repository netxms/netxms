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
package org.netxms.ui.eclipse.jobs;

import org.eclipse.ui.IWorkbenchPart;
import org.netxms.client.NXCSession;
import org.netxms.client.server.ServerJobIdUpdater;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.MessageBar;

/**
 * Console job that executes server side job and waits for its completion.
 * Callers must call start() instead of schedule() for correct execution.
 */
public abstract class ConsoleJobCallingServerJob extends ConsoleJob implements ServerJobIdUpdater
{
   private NXCSession session = null;
   private int serverJobId = 0;
   private boolean jobCanceled = false;

   /**
    * Create new job.
    * 
    * @param name Job name
    * @param wbPart Workbench part or null
    * @param pluginId Plugin ID or null
    */
   public ConsoleJobCallingServerJob(String name, IWorkbenchPart wbPart, String pluginId)
   {
      this(name, wbPart, pluginId, null);
   }

   /**
    * Create new job.
    * 
    * @param name Job name
    * @param wbPart Workbench part or null
    * @param pluginId Plugin ID or null
    * @param messageBar Message bar or null
    */
   public ConsoleJobCallingServerJob(String name, IWorkbenchPart wbPart, String pluginId, MessageBar messageBar)
   {
      super(name, wbPart, pluginId, messageBar);
      session = (NXCSession)ConsoleSharedData.getSession();
   }

   /* (non-Javadoc)
    * @see org.eclipse.core.runtime.jobs.Job#canceling()
    */
   @Override
   protected void canceling()
   {
      jobCanceled = true;
      try
      {
         session.cancelServerJob(serverJobId);
      }
      catch(Exception e)
      {
         Activator.logError("Failed to cancel job", e); //$NON-NLS-1$
      }
      super.canceling();
   }
 
   public void setJobIdCallback(int id)
   {
      serverJobId = id;
   }

   public boolean isCanceled()
   {
      return jobCanceled;
   }
}
