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
package org.netxms.ui.eclipse.snmp.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.views.helpers.SnmpTrapMonitorFilter;
import org.netxms.ui.eclipse.snmp.views.helpers.SnmpTrapMonitorLabelProvider;
import org.netxms.ui.eclipse.views.AbstractTraceView;

/**
 * SNMP trap monitor
 */
public class SnmpTrapMonitor extends AbstractTraceView implements SessionListener
{
	public static final String ID = "org.netxms.ui.eclipse.snmp.views.SnmpTrapMonitor"; //$NON-NLS-1$
	
	public static final int COLUMN_TIMESTAMP = 0;
	public static final int COLUMN_SOURCE_IP = 1;
	public static final int COLUMN_SOURCE_NODE = 2;
	public static final int COLUMN_OID = 3;
	public static final int COLUMN_VARBINDS = 4;
	
	private NXCSession session;
	
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
			new ConsoleJob(Messages.SnmpTrapMonitor_SubscribeJob_Title, null, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					session.subscribe(NXCSession.CHANNEL_SNMP_TRAPS);
				}
				
				@Override
				protected String getErrorMessage()
				{
					return Messages.SnmpTrapMonitor_SubscribeJob_Error;
				}
			}.start();
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.views.AbstractTraceView#setupViewer(org.eclipse.jface.viewers.TableViewer)
	 */
	@Override
	protected void setupViewer(TableViewer viewer)
	{
		addColumn(Messages.SnmpTrapMonitor_ColTime, 150);
		addColumn(Messages.SnmpTrapMonitor_ColSourceIP, 120);
		addColumn(Messages.SnmpTrapMonitor_ColSourceNode, 200);
		addColumn(Messages.SnmpTrapMonitor_ColOID, 200);
		addColumn(Messages.SnmpTrapMonitor_ColVarbinds, 600);
		
		viewer.setLabelProvider(new SnmpTrapMonitorLabelProvider());
		setFilter(new SnmpTrapMonitorFilter());
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
		return "SnmpTrapMonitor"; //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(final SessionNotification n)
	{
		if (n.getCode() == NXCNotification.NEW_SNMP_TRAP)
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
		session.removeListener(this);
		
		new ConsoleJob(Messages.SnmpTrapMonitor_UnsubscribeJob_Title, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.unsubscribe(NXCSession.CHANNEL_SNMP_TRAPS);
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.SnmpTrapMonitor_UnsubscribeJob_Error;
			}
		}.start();
		super.dispose();
	}
}
