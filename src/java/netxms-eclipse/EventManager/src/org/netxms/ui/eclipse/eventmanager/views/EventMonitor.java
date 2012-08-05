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
package org.netxms.ui.eclipse.eventmanager.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.eventmanager.Activator;
import org.netxms.ui.eclipse.eventmanager.Messages;
import org.netxms.ui.eclipse.eventmanager.views.helpers.EventLabelProvider;
import org.netxms.ui.eclipse.eventmanager.views.helpers.EventMonitorFilter;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.views.AbstractTraceView;

/**
 * Event monitor
 */
public class EventMonitor extends AbstractTraceView implements SessionListener
{
	public static final String ID = "org.netxms.ui.eclipse.eventmanager.views.EventMonitor"; //$NON-NLS-1$
	
	public static final int COLUMN_TIMESTAMP = 0;
	public static final int COLUMN_SOURCE = 1;
	public static final int COLUMN_SEVERITY = 2;
	public static final int COLUMN_EVENT = 3;
	public static final int COLUMN_MESSAGE = 4;
	
	private NXCSession session;
	private Action actionShowColor; 
	private Action actionShowIcons;
	private EventLabelProvider labelProvider;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		session = (NXCSession)ConsoleSharedData.getSession();
		session.addListener(this);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.views.AbstractTraceView#setupViewer(org.eclipse.jface.viewers.TableViewer)
	 */
	@Override
	protected void setupViewer(final TableViewer viewer)
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
	 * @see org.netxms.ui.eclipse.views.AbstractTraceView#createActions()
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
	 * @see org.netxms.ui.eclipse.views.AbstractTraceView#fillLocalPullDown(org.eclipse.jface.action.IMenuManager)
	 */
	@Override
	protected void fillLocalPullDown(IMenuManager manager)
	{
		super.fillLocalPullDown(manager);
		manager.add(new Separator());
		manager.add(actionShowColor);
		manager.add(actionShowIcons);
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

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		final IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		ps.setValue("EventMonitor.showColor", labelProvider.isShowColor()); //$NON-NLS-1$
		ps.setValue("EventMonitor.showIcons", labelProvider.isShowIcons()); //$NON-NLS-1$
		
		session.removeListener(this);
		super.dispose();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.views.AbstractTraceView#getDialogSettings()
	 */
	@Override
	protected IDialogSettings getDialogSettings()
	{
		return Activator.getDefault().getDialogSettings();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.views.AbstractTraceView#getConfigPrefix()
	 */
	@Override
	protected String getConfigPrefix()
	{
		return "EventMonitor"; //$NON-NLS-1$
	}	
}
