/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.eventmanager.widgets;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.PlatformUI;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.eventmanager.Activator;
import org.netxms.ui.eclipse.eventmanager.Messages;
import org.netxms.ui.eclipse.eventmanager.widgets.helpers.SyslogLabelProvider;
import org.netxms.ui.eclipse.eventmanager.widgets.helpers.SyslogMonitorFilter;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.AbstractTraceWidget;

/**
 * Syslog trace widget
 */
public class SyslogTraceWidget extends AbstractTraceWidget implements SessionListener
{
	public static final int COLUMN_TIMESTAMP = 0;
	public static final int COLUMN_SOURCE = 1;
	public static final int COLUMN_SEVERITY = 2;
	public static final int COLUMN_FACILITY = 3;
	public static final int COLUMN_HOSTNAME = 4;
	public static final int COLUMN_TAG = 5;
	public static final int COLUMN_MESSAGE = 6;
	
	private NXCSession session;
	private Action actionShowColor;
	private Action actionShowIcons;
	private SyslogLabelProvider labelProvider;

	public SyslogTraceWidget(Composite parent, int style, IViewPart viewPart)
	{
		super(parent, style, viewPart);

		session = (NXCSession)ConsoleSharedData.getSession();
		session.addListener(this);
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				session.removeListener(SyslogTraceWidget.this);
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#setupViewer(org.eclipse.jface.viewers.TableViewer)
	 */
	@Override
	protected void setupViewer(TableViewer viewer)
	{
		labelProvider = new SyslogLabelProvider();
		viewer.setLabelProvider(labelProvider);
		
		final IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		labelProvider.setShowColor(ps.getBoolean("SyslogMonitor.showColor")); //$NON-NLS-1$
		labelProvider.setShowIcons(ps.getBoolean("SyslogMonitor.showIcons")); //$NON-NLS-1$
		
		addColumn(Messages.SyslogMonitor_ColTimestamp, 150);
		addColumn(Messages.SyslogMonitor_ColSource, 200);
		addColumn(Messages.SyslogMonitor_ColSeverity, 90);
		addColumn(Messages.SyslogMonitor_ColFacility, 90);
		addColumn(Messages.SyslogMonitor_ColHostName, 130);
		addColumn(Messages.SyslogMonitor_ColTag, 90);
		addColumn(Messages.SyslogMonitor_ColMessage, 600);
		
		setFilter(new SyslogMonitorFilter());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#getDialogSettings()
	 */
	@Override
	protected IDialogSettings getDialogSettings()
	{
		return Activator.getDefault().getDialogSettings();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#getConfigPrefix()
	 */
	@Override
	protected String getConfigPrefix()
	{
		return "SyslogMonitor"; //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#saveConfig()
	 */
	@Override
	protected void saveConfig()
	{
		super.saveConfig();

		final IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		ps.setValue("SyslogMonitor.showColor", labelProvider.isShowColor()); //$NON-NLS-1$
		ps.setValue("SyslogMonitor.showIcons", labelProvider.isShowIcons()); //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#createActions()
	 */
	@Override
	protected void createActions()
	{
		super.createActions();
		
		actionShowColor = new Action(Messages.SyslogMonitor_ShowStatusColors, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				labelProvider.setShowColor(actionShowColor.isChecked());
				refresh();
			}
		};
		actionShowColor.setChecked(labelProvider.isShowColor());
		
		actionShowIcons = new Action(Messages.SyslogMonitor_ShowStatusIcons, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				labelProvider.setShowIcons(actionShowIcons.isChecked());
				refresh();
			}
		};
		actionShowIcons.setChecked(labelProvider.isShowIcons());
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(final SessionNotification n)
	{
		if (n.getCode() == NXCNotification.NEW_SYSLOG_RECORD)
		{
			PlatformUI.getWorkbench().getDisplay().asyncExec(new Runnable() {
				@Override
				public void run()
				{
					addElement(n.getObject());
				}
			});
		}
	}

	/**
	 * @return the actionShowColor
	 */
	public Action getActionShowColor()
	{
		return actionShowColor;
	}

	/**
	 * @return the actionShowIcons
	 */
	public Action getActionShowIcons()
	{
		return actionShowIcons;
	}
}
