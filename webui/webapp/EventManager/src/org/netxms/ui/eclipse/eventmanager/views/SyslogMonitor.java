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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.eventmanager.Activator;
import org.netxms.ui.eclipse.eventmanager.views.helpers.SyslogLabelProvider;
import org.netxms.ui.eclipse.eventmanager.views.helpers.SyslogMonitorFilter;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.views.AbstractTraceView;

/**
 * Event monitor
 */
public class SyslogMonitor extends AbstractTraceView implements SessionListener
{
	public static final String ID = "org.netxms.ui.eclipse.eventmanager.views.SyslogMonitor";
	
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
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
	 */
	@Override
	public void init(IViewSite site, IMemento memento) throws PartInitException
	{
		super.init(site, memento);
		if (memento != null)
		{
			new ConsoleJob("Subscribing to syslog events", null, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					session.subscribe(NXCSession.CHANNEL_SYSLOG);
				}
				
				@Override
				protected String getErrorMessage()
				{
					return "Cannot subscribe to syslog events";
				}
			}.start();
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.views.AbstractTraceView#setupViewer(org.eclipse.jface.viewers.TableViewer)
	 */
	@Override
	protected void setupViewer(final TableViewer viewer)
	{
		labelProvider = new SyslogLabelProvider();
		viewer.setLabelProvider(labelProvider);
		
		final IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		labelProvider.setShowColor(ps.getBoolean("SyslogMonitor.showColor"));
		labelProvider.setShowIcons(ps.getBoolean("SyslogMonitor.showIcons"));
		
		addColumn("Timestamp", 150);
		addColumn("Source", 200);
		addColumn("Severity", 90);
		addColumn("Facility", 90);
		addColumn("Host Name", 130);
		addColumn("Tag", 90);
		addColumn("Message", 600);
		
		setFilter(new SyslogMonitorFilter());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.views.AbstractTraceView#createActions()
	 */
	@Override
	protected void createActions()
	{
		super.createActions();
		
		actionShowColor = new Action("Show status &colors", Action.AS_CHECK_BOX) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				labelProvider.setShowColor(actionShowColor.isChecked());
				refresh();
			}
		};
		actionShowColor.setChecked(labelProvider.isShowColor());
		
		actionShowIcons = new Action("Show status &icons", Action.AS_CHECK_BOX) {
			private static final long serialVersionUID = 1L;

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

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		final IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		ps.setValue("SyslogMonitor.showColor", labelProvider.isShowColor());
		ps.setValue("SyslogMonitor.showIcons", labelProvider.isShowIcons());
		
		session.removeListener(this);
		
		new ConsoleJob("Unsubscribe from syslog events", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.unsubscribe(NXCSession.CHANNEL_SYSLOG);
			}
			
			@Override
			protected String getErrorMessage()
			{
				return "Cannot unsubscribe from syslog events";
			}
		}.start();
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
		return "SyslogMonitor";
	}	
}
