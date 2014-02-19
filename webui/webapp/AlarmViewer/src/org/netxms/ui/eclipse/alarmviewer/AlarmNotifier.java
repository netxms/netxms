/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.alarmviewer;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.URL;
import java.nio.channels.FileChannel;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.client.service.JavaScriptExecutor;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.Session;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCException;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.ui.eclipse.console.DownloadServiceHandler;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Alarm notifier
 */
public class AlarmNotifier
{
   private static NXCListener listener = null;
   private NXCSession session;
   public static String[] severityArray = { "NORMAL", "WARNING", "MINOR", "MAJOR", "CRITICAL" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$
   private IPreferenceStore ps;
   private URL workspaceUrl;

   /**
    * Get cache instance
    * 
    * @return
    */
   public static AlarmNotifier getInstance()
   {
      return (AlarmNotifier)ConsoleSharedData.getProperty("AlarmNotifier");
   }

   /**
    * Attach session to cache
    * 
    * @param session
    */
   public static void attachSession(final NXCSession session, final Display display)
   {
      AlarmNotifier instance = new AlarmNotifier();
      instance.init(session, display);
      ConsoleSharedData.setProperty("AlarmNotifier", instance);
   }

   /**
    * Initialize alarm notifier
    */
   public void init(final NXCSession session, final Display display)
   {
      this.session = session;
      ps = Activator.getDefault().getPreferenceStore();
      workspaceUrl = Platform.getInstanceLocation().getURL();
      // Check that required alarm melodies are present localy.
      checkMelodies(session);

      listener = new NXCListener() {
         @Override
         public void notificationHandler(final SessionNotification n)
         {
            if ((n.getCode() == NXCNotification.NEW_ALARM) || (n.getCode() == NXCNotification.ALARM_CHANGED))
            {
               display.asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     processNewAlarm((Alarm)n.getObject());
                  }
               });
            }
         }
      };
      session.addListener(listener);
   }

   /**
    * Check if required melodies exist locally and download them from server if required.
    */
   private void checkMelodies(NXCSession session)
   {
      URL workspaceUrl = Platform.getInstanceLocation().getURL();
      for(int i = 0; i < 5; i++)
      {
         getMelodyAndDownloadIfRequired(session, workspaceUrl, severityArray[i]);
      }
   }

   private String getMelodyAndDownloadIfRequired(NXCSession session, URL workspaceUrl, String severity)
   {
      String melodyName = ps.getString("ALARM_NOTIFIER.MELODY." + severity);//$NON-NLS-1$
      if (!checkMelodyExists(melodyName, workspaceUrl))
      {
         try
         {
            File f = new File(workspaceUrl.getPath(), melodyName);
            File fileContent = session.downloadFileFromServer(melodyName);
            f.createNewFile();
            FileChannel src = null;
            FileChannel dest = null;
            if (fileContent != null)
            {
               try
               {
                  src = new FileInputStream(fileContent).getChannel();
                  dest = new FileOutputStream(f).getChannel();
                  dest.transferFrom(src, 0, src.size());
               }
               catch(IOException e)
               {
                  System.out.println("Not possible to copy inside new file content.");
               }
               finally
               {
                  src.close();
                  dest.close();
               }
            }
         }
         catch(IOException | NXCException e)
         {
            melodyName = "";
            ps.setValue("ALARM_NOTIFIER.MELODY." + severity, "");
            new UIJob("Send error that sound file is not found") {
               @Override
               public IStatus runInUIThread(IProgressMonitor monitor)
               {
                  MessageDialogHelper
                        .openError(
                              getDisplay().getActiveShell(),
                              "Melody does not exist.",
                              "Melody was not found "
                                    + "locally and it was not possible to download it from server. Melody is removed and will not be played. Error: "
                                    + e.getMessage());
                  return Status.OK_STATUS;
               }
            }.schedule();
         }
      }
      return melodyName;
   }

   private boolean checkMelodyExists(String melodyName, URL workspaceUrl)
   {
      if ((!melodyName.equals("")) && (workspaceUrl != null))
      {
         File f = new File(workspaceUrl.getPath(), melodyName);
         return f.isFile();
      }
      else
      {
         return true;
      }
   }

   /**
    * Stop alarm notifier
    */
   public static void stop()
   {
      Session session = ConsoleSharedData.getSession();
      if ((session != null) && (listener != null))
         session.removeListener(listener);
   }

   /**
    * Process new alarm
    */
   private void processNewAlarm(final Alarm alarm)
   {
      if (alarm.getState() != Alarm.STATE_OUTSTANDING)
         return;

      String fileName = getMelodyAndDownloadIfRequired(session, workspaceUrl, severityArray[alarm.getCurrentSeverity()]); //$NON-NLS-1$

      if (!fileName.equals("") && fileName != null)
      {
         JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
         File localFile = new File(workspaceUrl.getPath(), fileName);
         String id = localFile.getAbsolutePath();
         DownloadServiceHandler.addDownload(id, fileName, localFile, "audio/wav");//$NON-NLS-1$
         StringBuilder js = new StringBuilder();
         js.append("var audio = new Audio('");//$NON-NLS-1$
         js.append(DownloadServiceHandler.createDownloadUrl(id));
         js.append("');");//$NON-NLS-1$
         js.append("audio.play();");//$NON-NLS-1$  
         executor.execute(js.toString());
      }
   }
}
