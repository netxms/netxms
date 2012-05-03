/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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

import java.util.HashMap;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
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
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.alarmviewer.dialogs.AlarmReminderDialog;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Alarm notifier
 *
 */
public class AlarmNotifier
{
	private static NXCListener listener = null;
	private static Map<Long, Integer> alarmStates = new HashMap<Long, Integer>();
	private static int outstandingAlarms = 0;
	private static long lastReminderTime = 0;
	
	/**
	 * Initialize alarm notifier
	 */
	public static void init(NXCSession session)
	{
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
				if ((n.getCode() == NXCNotification.NEW_ALARM) ||
				    (n.getCode() == NXCNotification.ALARM_CHANGED))
				{
					processNewAlarm((Alarm)n.getObject());
				}
				else if ((n.getCode() == NXCNotification.ALARM_TERMINATED) ||
				         (n.getCode() == NXCNotification.ALARM_DELETED))
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
					if (ps.getBoolean("OUTSTANDING_ALARMS_REMINDER") && 
							(outstandingAlarms > 0) && 
							(lastReminderTime + 300000 <= currTime))
					{
						Display.getDefault().syncExec(new Runnable() {
							@Override
							public void run()
							{
								AlarmReminderDialog dlg = new AlarmReminderDialog(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell());
								dlg.open();
							}
						});
						lastReminderTime = currTime;
					}
				}
			}
		}, "AlarmReminderThread");
		thread.setDaemon(true);
		thread.start();
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
				outstandingAlarms--;
		}
		alarmStates.put(alarm.getId(), alarm.getState());
		
		if (alarm.getState() != Alarm.STATE_OUTSTANDING)
			return;

		if (outstandingAlarms == 0)
			lastReminderTime = System.currentTimeMillis();
		outstandingAlarms++;
		
		if (!Activator.getDefault().getPreferenceStore().getBoolean("SHOW_TRAY_POPUPS"))
			return;
		
		final TrayItem trayIcon = ConsoleSharedData.getTrayIcon();
		if (trayIcon != null)
		{
			new UIJob("Create alarm popup") { //$NON-NLS-1$
				@Override
				public IStatus runInUIThread(IProgressMonitor monitor)
				{
					final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
					final GenericObject object = session.findObjectById(alarm.getSourceObjectId());
					
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
						tip.setText(Messages.AlarmNotifier_ToolTip_Header + StatusDisplayInfo.getStatusText(alarm.getCurrentSeverity()) + ")"); //$NON-NLS-2$
						tip.setMessage(((object != null) ? object.getObjectName() : Long.toString(alarm.getSourceObjectId())) + ": " + alarm.getMessage()); //$NON-NLS-1$
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
