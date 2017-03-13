/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Raden Solutions
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
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.events.Alarm;
import org.netxms.ui.eclipse.console.DownloadServiceHandler;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Alarm notifier
 */
public class AlarmNotifier
{
   public static final String[] SEVERITY_TEXT = { "NORMAL", "WARNING", "MINOR", "MAJOR", "CRITICAL", "REMINDER" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$
   
   private SessionListener listener = null;
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
            checkSounds();
         }
      });
      
      listener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if ((n.getCode() == SessionNotification.NEW_ALARM) || (n.getCode() == SessionNotification.ALARM_CHANGED))
            {
               processNewAlarm((Alarm)n.getObject());
            }
         }
      };
      session.addListener(listener);
   }

   /**
    * Check if required sounds exist locally and download them from server if required.
    */
   private void checkSounds()
   {
      for(String s : SEVERITY_TEXT)
      {
         getSoundAndDownloadIfRequired(s);
      }
   }

   /**
    * Get sound name for given severity
    * 
    * @param severity
    * @return
    */
   private String getSoundName(final String severity)
   {
      final String[] result = new String[1];
      display.syncExec(new Runnable() {
         @Override
         public void run()
         {
            result[0] = ps.getString("ALARM_NOTIFIER.MELODY." + severity);//$NON-NLS-1$
         }
      });
      return result[0];
   }

   /**
    * @param workspaceUrl
    * @param severity
    * @return
    */
   private String getSoundAndDownloadIfRequired(final String severity)
   {
      String soundName = getSoundName("ALARM_NOTIFIER.MELODY." + severity);//$NON-NLS-1$
      if (soundName.isEmpty())
         return null;
      
      if (!isSoundExists(soundName))
      {
         try
         {
            File fileContent = session.downloadFileFromServer(soundName);
            if (fileContent != null)
            {
               FileInputStream src = null;
               FileOutputStream dest = null;
               try
               {
                  src = new FileInputStream(fileContent);
                  File f = new File(workspaceUrl.getPath(), soundName);
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
            else
            {
               Activator.logError("Cannot download sound file " + soundName + " from server");
               soundName = null; // download failure
            }
         }
         catch(final Exception e)
         {
            soundName = null;
            display.syncExec(new Runnable() {
               @Override
               public void run()
               {
                  ps.setValue("ALARM_NOTIFIER.MELODY." + severity, ""); //$NON-NLS-1$ //$NON-NLS-2$
                  MessageDialogHelper
                        .openError(
                              display.getActiveShell(),
                              Messages.get().AlarmNotifier_Error,
                              String.format(Messages.get().AlarmNotifier_SoundPlayError,
                                    e.getMessage()));
               }
            });
         }
      }
      return soundName;
   }

   /**
    * @param name
    * @return
    */
   private boolean isSoundExists(String name)
   {
      if (!name.isEmpty() && (workspaceUrl != null))
      {
         File f = new File(workspaceUrl.getPath(), name);
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
         fileName = getSoundAndDownloadIfRequired(SEVERITY_TEXT[alarm.getCurrentSeverity().getValue()]);
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
         String id = "audio-" + fileName;
         DownloadServiceHandler.addDownload(id, fileName, localFile, "audio/wav"); //$NON-NLS-1$
         StringBuilder js = new StringBuilder();
         js.append("var testAudio = document.createElement('audio');");
         js.append("if (testAudio.canPlayType !== undefined)");
         js.append("{");
         js.append("var audio = new Audio('");//$NON-NLS-1$
         js.append(DownloadServiceHandler.createDownloadUrl(id));
         js.append("');");//$NON-NLS-1$
         js.append("audio.play();");//$NON-NLS-1$  
         js.append("}");
         executor.execute(js.toString());
      }
   }
}
