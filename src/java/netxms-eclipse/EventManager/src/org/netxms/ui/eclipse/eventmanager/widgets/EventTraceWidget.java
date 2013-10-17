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
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.eventmanager.Activator;
import org.netxms.ui.eclipse.eventmanager.Messages;
import org.netxms.ui.eclipse.eventmanager.widgets.helpers.EventLabelProvider;
import org.netxms.ui.eclipse.eventmanager.widgets.helpers.EventMonitorFilter;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.AbstractTraceWidget;

/**
 * Event trace widget
 */
public class EventTraceWidget extends AbstractTraceWidget implements SessionListener
{
	public static final int COLUMN_TIMESTAMP = 0;
	public static final int COLUMN_SOURCE = 1;
	public static final int COLUMN_SEVERITY = 2;
	public static final int COLUMN_EVENT = 3;
	public static final int COLUMN_MESSAGE = 4;
	
	private NXCSession session;
	private Action actionShowColor; 
	private Action actionShowIcons;
	private EventLabelProvider labelProvider;

	/**
	 * @param parent
	 * @param style
	 * @param viewPart
	 */
	public EventTraceWidget(Composite parent, int style, IViewPart viewPart)
	{
		super(parent, style, viewPart);

		session = (NXCSession)ConsoleSharedData.getSession();
		session.addListener(this);
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				session.removeListener(EventTraceWidget.this);
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#setupViewer(org.eclipse.jface.viewers.TableViewer)
	 */
	@Override
	protected void setupViewer(TableViewer viewer)
	{
		labelProvider = new EventLabelProvider();
		viewer.setLabelProvider(labelProvider);
		
		final IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		labelProvider.setShowColor(ps.getBoolean("EventMonitor.showColor")); //$NON-NLS-1$
		labelProvider.setShowIcons(ps.getBoolean("EventMonitor.showIcons")); //$NON-NLS-1$
		
		addColumn(Messages.EventMonitor_ColTimestamp, 150);
		addColumn(Messages.EventMonitor_ColSource, 200);
		addColumn(Messages.EventMonitor_ColSeverity, 90);
		addColumn(Messages.EventMonitor_ColEvent, 200);
		addColumn(Messages.EventMonitor_ColMessage, 600);
		
		setFilter(new EventMonitorFilter());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#saveConfig()
	 */
	@Override
	protected void saveConfig()
	{
		super.saveConfig();
		
		final IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		ps.setValue("EventMonitor.showColor", labelProvider.isShowColor());
		ps.setValue("EventMonitor.showIcons", labelProvider.isShowIcons());
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
		return "EventMonitor"; //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#createActions()
	 */
	@Override
	protected void createActions()
	{
		super.createActions();
		
		actionShowColor = new Action(Messages.EventMonitor_ShowStatusColors, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				labelProvider.setShowColor(actionShowColor.isChecked());
				refresh();
			}
		};
		actionShowColor.setChecked(labelProvider.isShowColor());
		
		actionShowIcons = new Action(Messages.EventMonitor_ShowStatusIcons, Action.AS_CHECK_BOX) {
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
		if (n.getCode() == NXCNotification.NEW_EVENTLOG_RECORD)
		{
			runInUIThread(new Runnable() {
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
