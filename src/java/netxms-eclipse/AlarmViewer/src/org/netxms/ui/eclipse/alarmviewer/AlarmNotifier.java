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
import java.util.HashMap;
import java.util.Map;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.Clip;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.Line;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.ToolTip;
import org.eclipse.swt.widgets.TrayItem;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.Session;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.alarmviewer.dialogs.AlarmReminderDialog;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Alarm notifier
 */
public class AlarmNotifier
{
   public static String[] severityArray = { "NORMAL", "WARNING", "MINOR", "MAJOR", "CRITICAL" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$
   
   private static NXCListener listener = null;
   private static Map<Long, Integer> alarmStates = new HashMap<Long, Integer>();
   private static int outstandingAlarms = 0;
   private static long lastReminderTime = 0;
   private static NXCSession session;
   private static IPreferenceStore ps;
   private static URL workspaceUrl;

   /**
    * Initialize alarm notifier
    */
   public static void init(NXCSession session)
   {
      AlarmNotifier.session = session;
      ps = Activator.getDefault().getPreferenceStore();
      workspaceUrl = Platform.getInstanceLocation().getURL();
      // Check that required alarm melodies are present locally.
      checkMelodies();

      lastReminderTime = System.currentTimeMillis();

      try
      {
         Map<Long, Alarm> alarms = session.getAlarms();
         for(Alarm a : alarms.values())
         {
            alarmStates.put(a.getId(), a.getState());
            if (a.getState() == Alarm.STATE_OUTSTANDING)
               outstandingAlarms++;
         }
      }
      catch(Exception e)
      {
      }

      listener = new NXCListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if ((n.getCode() == NXCNotification.NEW_ALARM) || (n.getCode() == NXCNotification.ALARM_CHANGED))
            {
               processNewAlarm((Alarm)n.getObject());
            }
            else if ((n.getCode() == NXCNotification.ALARM_TERMINATED) || (n.getCode() == NXCNotification.ALARM_DELETED))
            {
               Integer state = alarmStates.get(((Alarm)n.getObject()).getId());
               if (state != null)
               {
                  if (state == Alarm.STATE_OUTSTANDING)
                     outstandingAlarms--;
                  alarmStates.remove(((Alarm)n.getObject()).getId());
               }
            }
         }
      };
      session.addListener(listener);

      Thread thread = new Thread(new Runnable() {
         @Override
         public void run()
         {
            while(true)
            {
               try
               {
                  Thread.sleep(10000);
               }
               catch(InterruptedException e)
               {
               }

               IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
               long currTime = System.currentTimeMillis();
               if (ps.getBoolean("OUTSTANDING_ALARMS_REMINDER") && //$NON-NLS-1$
                     (outstandingAlarms > 0) && (lastReminderTime + 300000 <= currTime))
               {
                  Display.getDefault().syncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        AlarmReminderDialog dlg = new AlarmReminderDialog(PlatformUI.getWorkbench().getActiveWorkbenchWindow()
                              .getShell());
                        dlg.open();
                     }
                  });
                  lastReminderTime = currTime;
               }
            }
         }
      }, "AlarmReminderThread"); //$NON-NLS-1$
      thread.setDaemon(true);
      thread.start();
   }

   /**
    * Check if required melodies exist locally and download them from server if required.
    */
   private static void checkMelodies()
   {
      for(int i = 0; i < 5; i++)
      {
         getMelodyAndDownloadIfRequired(severityArray[i]);
      }
   }

   /**
    * @param severity
    * @return
    */
   private static String getMelodyAndDownloadIfRequired(String severity)
   {
      String melodyName = ps.getString("ALARM_NOTIFIER.MELODY." + severity);//$NON-NLS-1$
      if (!isMelodyExists(melodyName, workspaceUrl))
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
                  Activator.logError("Cannot copy sound file", e);
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
            Display.getDefault().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  MessageDialogHelper
                  .openError(
                        Display.getDefault().getActiveShell(),
                        Messages.get().AlarmNotifier_ErrorMelodynotExists,
                        Messages.get().AlarmNotifier_ErrorMelodyNotExistsDescription
                              + e.getLocalizedMessage());
               }
            });
         }
      }
      return melodyName;
   }

   /**
    * @param melodyName
    * @param workspaceUrl
    * @return
    */
   private static boolean isMelodyExists(String melodyName, URL workspaceUrl)
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
   private static void processNewAlarm(final Alarm alarm)
   {
      Integer state = alarmStates.get(alarm.getId());
      if (state != null)
      {
         if (state == Alarm.STATE_OUTSTANDING)
         {
            outstandingAlarms--;
         }
      }
      alarmStates.put(alarm.getId(), alarm.getState());

      if (alarm.getState() != Alarm.STATE_OUTSTANDING)
         return;

      String fileName = getMelodyAndDownloadIfRequired(severityArray[alarm.getCurrentSeverity()]); //$NON-NLS-1$

      if ((fileName != null) && !fileName.isEmpty())
      {
         try
         {
            Clip sound = (Clip) AudioSystem.getLine(new Line.Info(Clip.class));
            sound.open(AudioSystem.getAudioInputStream(new File(workspaceUrl.getPath(), fileName).getAbsoluteFile()));
            sound.start();
            while(!sound.isRunning())
               Thread.sleep(10);
            while(sound.isRunning())
            {
               Thread.sleep(10);
            }
            sound.close();
         }
         catch(final Exception e)
         {
            Display.getDefault().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  MessageDialogHelper.openError(Display.getDefault().getActiveShell(), Messages.get().AlarmNotifier_ErrorPlayingSound,
                        Messages.get().AlarmNotifier_ErrorPlayingSoundDescription + e.getMessage());
               }
            });
         }
      }

      if (outstandingAlarms == 0)
         lastReminderTime = System.currentTimeMillis();
      outstandingAlarms++;

      if (!Activator.getDefault().getPreferenceStore().getBoolean("SHOW_TRAY_POPUPS")) //$NON-NLS-1$
         return;

      final TrayItem trayIcon = ConsoleSharedData.getTrayIcon();
      if (trayIcon != null)
      {
         new UIJob("Create alarm popup") { //$NON-NLS-1$
            @Override
            public IStatus runInUIThread(IProgressMonitor monitor)
            {
               final AbstractObject object = session.findObjectById(alarm.getSourceObjectId());

               int severityFlag;
               if (alarm.getCurrentSeverity() == Severity.NORMAL)
               {
                  severityFlag = SWT.ICON_INFORMATION;
               }
               else if (alarm.getCurrentSeverity() == Severity.CRITICAL)
               {
                  severityFlag = SWT.ICON_ERROR;
               }
               else
               {
                  severityFlag = SWT.ICON_WARNING;
               }

               IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
               if (window == null)
               {
                  IWorkbenchWindow[] wl = PlatformUI.getWorkbench().getWorkbenchWindows();
                  if (wl.length > 0)
                     window = wl[0];
               }
               if (window != null)
               {
                  final ToolTip tip = new ToolTip(window.getShell(), SWT.BALLOON | severityFlag);
                  tip.setText(Messages.get().AlarmNotifier_ToolTip_Header
                        + StatusDisplayInfo.getStatusText(alarm.getCurrentSeverity()) + ")"); //$NON-NLS-1$ //$NON-NLS-1$
                  tip.setMessage(((object != null) ? object.getObjectName() : Long.toString(alarm.getSourceObjectId()))
                        + ": " + alarm.getMessage()); //$NON-NLS-1$
                  tip.setAutoHide(true);
                  trayIcon.setToolTip(tip);
                  tip.setVisible(true);
               }
               return Status.OK_STATUS;
            }
         }.schedule();
      }
   }
}
