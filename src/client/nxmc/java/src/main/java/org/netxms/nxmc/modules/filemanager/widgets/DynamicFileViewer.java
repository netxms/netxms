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
package org.netxms.nxmc.modules.filemanager.widgets;

import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.AgentFileData;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Dynamic file viewer - follows file changes
 */
public class DynamicFileViewer extends BaseFileViewer
{
   private final I18n i18n = LocalizationHelper.getI18n(DynamicFileViewer.class);
   private static Logger logger = LoggerFactory.getLogger(DynamicFileViewer.class);

   protected Job monitoringJob = null;
   protected Job restartJob = null;
   protected long nodeId = 0;
   protected String remoteFileName;
   protected NXCSession session = Registry.getSession();

   /**
    * @param parent
    * @param style
    * @param viewPart
    */
   public DynamicFileViewer(Composite parent, int style, View view)
   {
      super(parent, style, view);

      setScrollOnAppend(!scrollLock);

      final SessionListener sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if ((n.getCode() == SessionNotification.FILE_MONITORING_FAILED) && (n.getSubCode() == nodeId))
            {
               getDisplay().asyncExec(new Runnable() {
                  public void run()
                  {
                     restartTracking();
                  }
               });
            }
         }
      };
      session.addListener(sessionListener);

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            session.removeListener(sessionListener);
            stopTracking();
         }
      });
   }

   /**
    * Start file change tracking
    * 
    * @param nodeId
    * @param fileId
    */
   public void startTracking(final UUID monitorId, final long nodeId, final String remoteFileName)
   {
      if (restartJob != null)
      {
         restartJob.cancel();
         restartJob = null;
      }

      if (monitoringJob != null)
      {
         monitoringJob.cancel();
         monitoringJob = null;
      }

      view.clearMessages();

      this.nodeId = nodeId;
      this.remoteFileName = remoteFileName;

      setTextTopIndex();
      if (monitorId != null)
      {
         monitoringJob = new MonitoringJob(monitorId);
         monitoringJob.start();
      }
      else
      {
         restartTracking();
      }
   }

   /**
    * Stop tracking
    */
   public void stopTracking()
   {
      if (restartJob != null)
      {
         restartJob.cancel();
         restartJob = null;
      }

      if (monitoringJob != null)
      {
         monitoringJob.cancel();
         monitoringJob = null;
         nodeId = 0;
      }
   }

   /**
    * Restart tracking after failure
    */
   private void restartTracking()
   {
      if (monitoringJob != null)
      {
         monitoringJob.cancel();
         monitoringJob = null;
      }

      if (restartJob != null)
         restartJob.cancel();

      text.append("\n\n" + //$NON-NLS-1$
                  "----------------------------------------------------------------------\n" + //$NON-NLS-1$
                  i18n.tr("Connection with the agent has been lost. Attempting to reconnect...") +
                  "\n----------------------------------------------------------------------\n"); //$NON-NLS-1$

      restartJob = new Job(i18n.tr("Restart file tracking"), view) {
         private boolean running = true;

         @Override
         protected void canceling()
         {
            running = false;
         }

         @Override
         protected void run(final IProgressMonitor monitor) throws Exception
         {
            // Try to reconnect in every 20 seconds
            while(running)
            {
               try
               {
                  final AgentFileData file = session.downloadFileFromAgent(nodeId, remoteFileName, 1024, true, new ProgressListener() {
                     @Override
                     public void setTotalWorkAmount(long workTotal)
                     {
                        monitor.beginTask("Track file " + remoteFileName, (int)workTotal);
                     }

                     @Override
                     public void markProgress(long workDone)
                     {
                        monitor.worked((int)workDone);
                     }
                  });

                  // When successfully connected - display notification to client.
                  runInUIThread(new Runnable() {
                     @Override
                     public void run()
                     {
                        if (text.isDisposed())
                        {
                           running = false;
                           return;
                        }
                        
                        text.append("-------------------------------------------------------------------------------\n" + //$NON-NLS-1$
                                    i18n.tr("Connection with the agent restored.") +
                                    "\n-------------------------------------------------------------------------------\n\n"); //$NON-NLS-1$
                        append(loadFile(file.getFile()));
                        startTracking(file.getMonitorId(), nodeId, remoteFileName);
                     }
                  });
                  break;
               }
               catch(Exception e)
               {
               }
               Thread.sleep(20000);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot restart file change tracking");
         }
      };
      restartJob.setUser(false);
      restartJob.setSystem(true);
      restartJob.start();
   }

   /**
    * File monitoring job
    */
   private class MonitoringJob extends Job
   {
      private boolean tracking = true;
      private UUID monitorId;

      public MonitoringJob(UUID monitorId)
      {
         super(LocalizationHelper.getI18n(DynamicFileViewer.class).tr("Monitoring changes in file {0} on node {1}", remoteFileName, session.getObjectName(nodeId)), view);
         this.monitorId = monitorId;
         setUser(false);
         setSystem(true);
      }

      /**
       * @see org.netxms.nxmc.base.jobs.Job#canceling()
       */
      @Override
      protected void canceling()
      {
         tracking = false;
      }

      /**
       * @see org.netxms.nxmc.base.jobs.Job#run(org.eclipse.core.runtime.IProgressMonitor)
       */
      @Override
      protected void run(IProgressMonitor monitor) throws Exception
      {
         while(tracking)
         {
            final String s = session.waitForFileUpdate(monitorId, 3000);
            if (s != null)
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (!text.isDisposed())
                     {
                        append(s);
                     }
                     else
                     {
                        tracking = false;
                     }
                  }
               });
            }
         }

         try
         {
            session.cancelFileMonitoring(monitorId);
         }
         catch(Exception e)
         {
            logger.warn(String.format("Cannot cancel file monitor with ID %s for node %s", monitorId.toString(), session.getObjectName(nodeId)), e);
         }
      }

      /**
       * @see org.netxms.nxmc.base.jobs.Job#getErrorMessage()
       */
      @Override
      protected String getErrorMessage()
      {
         return i18n.tr("Cannot start monitor for file {0} on node {1}", remoteFileName, session.getObjectName(nodeId));
      }
   }
}
