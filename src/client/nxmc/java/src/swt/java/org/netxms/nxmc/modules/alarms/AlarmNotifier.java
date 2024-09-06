/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.alarms;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicInteger;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.Clip;
import javax.sound.sampled.Line;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.ToolTip;
import org.eclipse.swt.widgets.TrayItem;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.BulkAlarmStateChangeData;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.windows.TrayIconManager;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.dialogs.AlarmReminderDialog;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Alarm notifier
 */
public class AlarmNotifier
{
   public static final String[] SEVERITY_TEXT = { "NORMAL", "WARNING", "MINOR", "MAJOR", "CRITICAL", "REMINDER" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$
   private static final String LOCAL_SOUND_ID = "AlarmNotifier.LocalSound";
   private static final int MAX_PLAY_QUEUE_LINE = 5;

   private static Logger logger = LoggerFactory.getLogger(AlarmNotifier.class);
   private static SessionListener listener = null;
   private static Map<Long, Integer> alarmStates = new HashMap<Long, Integer>();
   private static int outstandingAlarms = 0;
   private static long lastReminderTime = 0;
   private static NXCSession session;
   private static PreferenceStore ps;
   private static File soundFilesDirectory;
   private static LinkedBlockingQueue<String> soundQueue = new LinkedBlockingQueue<String>(4);
   private static AtomicInteger trayPopupCount = new AtomicInteger(0);
   private static AtomicInteger trayPopupError = new AtomicInteger(0);
   private static Map<Long, Long> alarmNotificationTimestamp = new HashMap<Long, Long>();
   private static long lastNotificationTimestamp;

   /**
    * Initialize alarm notifier
    */
   public static void init(NXCSession session, Display display)
   {
      AlarmNotifier.session = session;
      ps = PreferenceStore.getInstance();
      soundFilesDirectory = new File(Registry.getStateDir(), "sounds");
      if (!soundFilesDirectory.isDirectory())
         soundFilesDirectory.mkdirs();

      checkSounds();

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
         logger.info(String.format("Received %d alarms from server (%d outstanding)", alarms.size(), outstandingAlarms));
      }
      catch(Exception e)
      {
         logger.error("Exception while initializing alarm notifier", e);
      }

      listener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if ((n.getCode() == SessionNotification.NEW_ALARM) || (n.getCode() == SessionNotification.ALARM_CHANGED))
            {
               processNewAlarm((Alarm)n.getObject());
            }
            else if ((n.getCode() == SessionNotification.ALARM_TERMINATED) || (n.getCode() == SessionNotification.ALARM_DELETED))
            {               
               Alarm a = (Alarm)n.getObject();
               Integer state = alarmStates.remove(a.getId());
               if ((state != null) && (state == Alarm.STATE_OUTSTANDING))
                  outstandingAlarms--;
            }
            else if (n.getCode() == SessionNotification.MULTIPLE_ALARMS_RESOLVED)
            {               
               BulkAlarmStateChangeData d = (BulkAlarmStateChangeData)n.getObject();
               for(Long id : d.getAlarms())
               {
                  Integer state = alarmStates.get(id);
                  if ((state != null) && (state == Alarm.STATE_OUTSTANDING))
                     outstandingAlarms--;
                  alarmStates.put(id, Alarm.STATE_RESOLVED);
               }
            }
            else if (n.getCode() == SessionNotification.MULTIPLE_ALARMS_TERMINATED)
            {               
               BulkAlarmStateChangeData d = (BulkAlarmStateChangeData)n.getObject();
               for(Long id : d.getAlarms())
               {
                  Integer state = alarmStates.remove(id);
                  if ((state != null) && (state == Alarm.STATE_OUTSTANDING))
                     outstandingAlarms--;
               }
            }
         }
      };
      session.addListener(listener);

      Thread reminderThread = new Thread(() -> {
         while(true)
         {
            try
            {
               Thread.sleep(10000);
            }
            catch(InterruptedException e)
            {
            }

            long currTime = System.currentTimeMillis();
            if (ps.getAsBoolean("AlarmNotifier.OutstandingAlarmsReminder", false) && (outstandingAlarms > 0) && (lastReminderTime + 300000 <= currTime))
            {
               Display.getDefault().syncExec(() -> {
                  soundQueue.offer(SEVERITY_TEXT[SEVERITY_TEXT.length - 1]);
                  AlarmReminderDialog dlg = new AlarmReminderDialog(null);
                  dlg.open();
               });
               lastReminderTime = currTime;
            }
         }
      }, "AlarmReminderThread");
      reminderThread.setDaemon(true);
      reminderThread.start();

      Thread playerThread = new Thread(new Runnable() {
         @Override
         public void run()
         {
            while(true)
            {
               String soundId;
               try
               {
                  soundId = soundQueue.take();
               }
               catch(InterruptedException e)
               {
                  continue;
               }
               
               Clip sound = null;
               try
               {
                  String fileName = getSoundAndDownloadIfRequired(soundId);
                  if (fileName != null)
                  {
                     sound = (Clip)AudioSystem.getLine(new Line.Info(Clip.class));
                     sound.open(AudioSystem.getAudioInputStream(new File(soundFilesDirectory, fileName).getAbsoluteFile()));
                     sound.start();
                     while(!sound.isRunning())
                        Thread.sleep(10);
                     while(sound.isRunning())
                     {
                        Thread.sleep(10);
                     }
                  }
               }
               catch(Exception e)
               {
                  logger.error("Exception in alarm sound player", e);
               }
               finally
               {
                  if ((sound != null) && sound.isOpen())
                     sound.close();
               }
            }
         }
      }, "AlarmSoundPlayer");
      playerThread.setDaemon(true);
      playerThread.start();
   }

   /**
    * Check if required sounds exist locally and download them from server if required.
    */
   private static void checkSounds()
   {
      for(String s : SEVERITY_TEXT)
      {
         getSoundAndDownloadIfRequired(s);
      }
   }

   /**
    * @param severity
    * @return
    */
   private static String getSoundAndDownloadIfRequired(String severity)
   {
      I18n i18n = LocalizationHelper.getI18n(AlarmNotifier.class);
      String soundName = ps.getAsString("AlarmNotifier.Sound." + severity);
      if ((soundName == null) || soundName.isEmpty())
         return null;

      if (!isSoundFileExist(soundName))
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
                  File f = new File(soundFilesDirectory, soundName);
                  f.createNewFile();
                  dest = new FileOutputStream(f);
                  FileChannel fcSrc = src.getChannel();
                  dest.getChannel().transferFrom(fcSrc, 0, fcSrc.size());
               }
               catch(IOException e)
               {
                  logger.error("Cannot copy sound file", e);
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
               logger.error("Cannot download sound file " + soundName + " from server");
               soundName = null; // download failure
            }
         }
         catch(final Exception e)
         {
            soundName = null;
            ps.set("AlarmNotifier.Sound." + severity, ""); //$NON-NLS-1$ //$NON-NLS-2$
            Display.getDefault().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  MessageDialogHelper.openError(
                        Display.getDefault().getActiveShell(),
                        i18n.tr("Missing Sound File"),
                        String.format(i18n.tr(
                              "Sound file is missing and cannot be downloaded from server. Sound is removed and will not be played again. Error details: %s"),
                              e.getLocalizedMessage()));
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
   private static boolean isSoundFileExist(String name)
   {
      if (name.isEmpty())
         return true;
      File f = new File(soundFilesDirectory, name);
      return f.isFile();
   }

   /**
    * Stop alarm notifier
    */
   public static void stop()
   {
      NXCSession session = Registry.getSession();
      if ((session != null) && (listener != null))
         session.removeListener(listener);
   }

   /**
    * Play sound for new view alarm
    * 
    * @param alarm new alarm
    */
   public static void playSounOnAlarm(final Alarm alarm, Display display)
   {  
      if(ps.getAsBoolean(LOCAL_SOUND_ID, false))
      {         
         long now = new Date().getTime();
         synchronized(alarmNotificationTimestamp)
         {
            boolean playSound = false;
            if ((now - lastNotificationTimestamp) > 2000)
            {
               playSound = true; 
               alarmNotificationTimestamp.clear();
            }
            else
            {
               Long time = alarmNotificationTimestamp.get(alarm.getId());
               if (time == null || ((now - time) > 500))
               {
                  playSound = true;                  
               }
            }
            
            if (playSound)
            {
               if (soundQueue.size() <= MAX_PLAY_QUEUE_LINE)
               {
                  lastNotificationTimestamp = now;
                  alarmNotificationTimestamp.put(alarm.getId(), now);
                  soundQueue.offer(SEVERITY_TEXT[alarm.getCurrentSeverity().getValue()]);
               }
               else
               {
                  logger.warn("Too many sound to play in queue");
               }
            }
         }
      }
   }

   /**
    * Process new alarm
    */
   private static void processNewAlarm(final Alarm alarm)
   {
      I18n i18n = LocalizationHelper.getI18n(AlarmNotifier.class);
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

      if (!ps.getAsBoolean(LOCAL_SOUND_ID, false))
      {
         if (soundQueue.size() <= MAX_PLAY_QUEUE_LINE)
         {
            soundQueue.offer(SEVERITY_TEXT[alarm.getCurrentSeverity().getValue()]);
         }
         else
         {
            logger.warn("Too many sound to play in queue");
         }
      }

      if (outstandingAlarms == 0)
         lastReminderTime = System.currentTimeMillis();
      outstandingAlarms++;

      if ((trayPopupError.get() > 0) || !ps.getAsBoolean("TrayIcon.ShowAlarmPopups", false))
         return;

      final TrayItem trayIcon = TrayIconManager.getTrayIcon();
      if (trayIcon == null)
         return;

      if (trayPopupCount.incrementAndGet() < 10)
      {
         Display.getDefault().asyncExec(new Runnable() {
            @Override
            public void run()
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

               Window window = Registry.getMainWindow();
               if (window != null)
               {
                  final ToolTip tip = new ToolTip(window.getShell(), SWT.BALLOON | severityFlag);
                  tip.setText(i18n.tr("NetXMS Alarm ({0})", StatusDisplayInfo.getStatusText(alarm.getCurrentSeverity())));
                  tip.setMessage(((object != null) ? object.getObjectName() : Long.toString(alarm.getSourceObjectId())) + ": "
                        + alarm.getMessage());
                  tip.setAutoHide(false);
                  trayIcon.setToolTip(tip);
                  tip.setVisible(true);
                  tip.getDisplay().timerExec(10000, new Runnable() {
                     @Override
                     public void run()
                     {
                        tip.dispose();
                     }
                  });
                  tip.addDisposeListener(new DisposeListener() {
                     @Override
                     public void widgetDisposed(DisposeEvent e)
                     {
                        trayPopupCount.decrementAndGet();
                     }
                  });
               }
            }
         });
      }
      else
      {
         // Too many notifications to show
         trayPopupCount.decrementAndGet();
         logger.info("Skipping alarm tray popup creation - too many consecutive alarms");

         if (trayPopupError.incrementAndGet() == 1)
         {
            Display.getDefault().asyncExec(new Runnable() { // $NON-NLS-1$
               @Override
               public void run()
               {
                  int severityFlag = SWT.ICON_INFORMATION;
   
                  Window window = Registry.getMainWindow();
                  if (window != null)
                  {
                     final ToolTip tip = new ToolTip(window.getShell(), SWT.BALLOON | severityFlag);
                     tip.setText(i18n.tr("Too many consecutive alarms"));
                     tip.setMessage(i18n.tr("Skipping alarm tray popup creation - too many consecutive alarms"));
                     tip.setAutoHide(false);
                     trayIcon.setToolTip(tip);
                     tip.setVisible(true);
                     tip.getDisplay().timerExec(10000, new Runnable() {
                        @Override
                        public void run()
                        {
                           tip.dispose();
                        }
                     });
                     tip.addDisposeListener(new DisposeListener() {
                        @Override
                        public void widgetDisposed(DisposeEvent e)
                        {
                           Display.getCurrent().timerExec(60000, new Runnable() {
                              @Override
                              public void run()
                              {
                                 trayPopupError.decrementAndGet();
                              }
                           });
                        }
                     });
                  }
               }
            });
         }
         else
         {
            trayPopupError.decrementAndGet();
         }
      }
   }
}
