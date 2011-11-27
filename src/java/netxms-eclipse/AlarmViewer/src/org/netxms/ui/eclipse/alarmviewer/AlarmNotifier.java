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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.swt.SWT;
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
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Alarm notifier
 *
 */
public class AlarmNotifier
{
	private static NXCListener listener = null;
	
	/**
	 * Initialize alarm notifier
	 */
	public static void init()
	{
		listener = new NXCListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if ((n.getCode() == NXCNotification.NEW_ALARM) ||
				    (n.getCode() == NXCNotification.ALARM_CHANGED))
					processNewAlarm((Alarm)n.getObject());
			}
		};
		Session session = ConsoleSharedData.getSession();
		if (session != null)
			session.addListener(listener);
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
		if (alarm.getState() != Alarm.STATE_OUTSTANDING)
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
