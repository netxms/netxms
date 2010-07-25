/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.shared.StatusDisplayInfo;

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
			public void notificationHandler(NXCNotification n)
			{
				if ((n.getCode() == NXCNotification.NEW_ALARM) ||
				    (n.getCode() == NXCNotification.ALARM_CHANGED))
					processNewAlarm((Alarm)n.getObject());
			}
		};
		NXCSession session = NXMCSharedData.getInstance().getSession();
		if (session != null)
			session.addListener(listener);
	}
	
	/**
	 * Stop alarm notifier
	 */
	public static void stop()
	{
		NXCSession session = NXMCSharedData.getInstance().getSession();
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
		
		final TrayItem trayIcon = NXMCSharedData.getInstance().getTrayIcon();
		if (trayIcon != null)
		{
			new UIJob("Create alarm popup") {
				@Override
				public IStatus runInUIThread(IProgressMonitor monitor)
				{
					final NXCSession session = NXMCSharedData.getInstance().getSession();
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
					
					final ToolTip tip = new ToolTip(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), SWT.BALLOON | severityFlag);
					tip.setText("NetXMS Alarm (" + StatusDisplayInfo.getStatusText(alarm.getCurrentSeverity()) + ")");
					tip.setMessage(((object != null) ? object.getObjectName() : Long.toString(alarm.getSourceObjectId())) + ": " + alarm.getMessage());
					tip.setAutoHide(true);
					trayIcon.setToolTip(tip);
					tip.setVisible(true);
					return Status.OK_STATUS;
				}
			}.schedule();
		}
	}
}
