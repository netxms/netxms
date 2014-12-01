/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.client.service.JavaScriptExecutor;
import org.eclipse.swt.widgets.Display;
import org.netxms.api.client.SessionNotification;
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
   public static final String[] severityArray = { "NORMAL", "WARNING", "MINOR", "MAJOR", "CRITICAL" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$
   
   private NXCListener listener = null;
   private Display display;
   private NXCSession session;
   private IPreferenceStore ps;
   private URL workspaceUrl;

   /**
    * Get cache instance
    * 
    * @return
    */
   public static AlarmNotifier getInstance()
   {
      return (AlarmNotifier)ConsoleSharedData.getProperty("AlarmNotifier"); //$NON-NLS-1$
   }

   /**
    * Attach session to cache
    * 
    * @param session
    */
   public static void attachSession(final NXCSession session, final Display display)
   {
      AlarmNotifier instance = new AlarmNotifier(session, display);
      ConsoleSharedData.setProperty(display, "AlarmNotifier", instance); //$NON-NLS-1$
   }

   /**
    * Initialize alarm notifier
    */
   private AlarmNotifier(final NXCSession session, final Display display)
   {
      this.session = session;
      this.display = display;
      
      display.syncExec(new Runnable() {
         @Override
         public void run()
         {
            ps = Activator.getDefault().getPreferenceStore();
            workspaceUrl = Platform.getInstanceLocation().getURL();

            // Check that required alarm melodies are present locally
            checkMelodies();
         }
      });
      
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
   private void checkMelodies()
   {
      for(int i = 0; i < 5; i++)
      {
         getMelodyAndDownloadIfRequired(severityArray[i]);
      }
   }

   /**
    * @param workspaceUrl
    * @param severity
    * @return
    */
   private String getMelodyAndDownloadIfRequired(String severity)
   {
      String melodyName = ps.getString("ALARM_NOTIFIER.MELODY." + severity);//$NON-NLS-1$
      if (!isMelodyExists(melodyName))
      {
         try
         {
            File fileContent = session.downloadFileFromServer(melodyName);
            if (fileContent != null)
            {
               FileInputStream src = null;
               FileOutputStream dest = null;
               try
               {
                  src = new FileInputStream(fileContent);
                  File f = new File(workspaceUrl.getPath(), melodyName);
                  f.createNewFile();
                  dest = new FileOutputStream(f);
                  FileChannel fcSrc = src.getChannel();
                  dest.getChannel().transferFrom(fcSrc, 0, fcSrc.size());
               }
               catch(IOException e)
               {
                  Activator.logError("Cannot copy sound file", e); //$NON-NLS-1$
               }
               finally
               {
                  if (src != null)
                     src.close();
                  if (dest != null)
                     dest.close();
               }
            }
         }
         catch(final Exception e)
         {
            melodyName = ""; //$NON-NLS-1$
            ps.setValue("ALARM_NOTIFIER.MELODY." + severity, ""); //$NON-NLS-1$ //$NON-NLS-2$
            MessageDialogHelper
                  .openError(
                        display.getActiveShell(),
                        Messages.get().AlarmNotifier_Error,
                        String.format(Messages.get().AlarmNotifier_SoundPlayError,
                              e.getMessage()));
         }
      }
      return melodyName;
   }

   /**
    * @param melodyName
    * @param workspaceUrl
    * @return
    */
   private boolean isMelodyExists(String melodyName)
   {
      if (!melodyName.isEmpty() && (workspaceUrl != null))
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
    * Process new alarm
    */
   private void processNewAlarm(final Alarm alarm)
   {
      if (alarm.getState() != Alarm.STATE_OUTSTANDING)
         return;

      String fileName;
      try
      {
         fileName = getMelodyAndDownloadIfRequired(severityArray[alarm.getCurrentSeverity().getValue()]);
      }
      catch(ArrayIndexOutOfBoundsException e)
      {
         Activator.logError("Invalid alarm severity", e); //$NON-NLS-1$
         fileName = null;
      }

      if ((fileName != null) && !fileName.isEmpty())
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
