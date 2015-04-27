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
package org.netxms.ui.eclipse.snmp.widgets;

import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.widgets.helpers.SnmpTrapMonitorFilter;
import org.netxms.ui.eclipse.snmp.widgets.helpers.SnmpTrapMonitorLabelProvider;
import org.netxms.ui.eclipse.widgets.AbstractTraceWidget;

/**
 * SNMP trap trace widget
 */
public class SnmpTrapTraceWidget extends AbstractTraceWidget implements SessionListener
{
	public static final int COLUMN_TIMESTAMP = 0;
	public static final int COLUMN_SOURCE_IP = 1;
	public static final int COLUMN_SOURCE_NODE = 2;
	public static final int COLUMN_OID = 3;
	public static final int COLUMN_VARBINDS = 4;
	
	private NXCSession session;
	
	/**
	 * @param parent
	 * @param style
	 * @param viewPart
	 */
	public SnmpTrapTraceWidget(Composite parent, int style, IViewPart viewPart)
	{
		super(parent, style, viewPart);
		session = (NXCSession)ConsoleSharedData.getSession();
		session.addListener(this);
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				session.removeListener(SnmpTrapTraceWidget.this);
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractTraceWidget#setupViewer(org.eclipse.jface.viewers.TableViewer)
	 */
	@Override
	protected void setupViewer(TableViewer viewer)
	{
		addColumn(Messages.get().SnmpTrapMonitor_ColTime, 150);
		addColumn(Messages.get().SnmpTrapMonitor_ColSourceIP, 120);
		addColumn(Messages.get().SnmpTrapMonitor_ColSourceNode, 200);
		addColumn(Messages.get().SnmpTrapMonitor_ColOID, 200);
		addColumn(Messages.get().SnmpTrapMonitor_ColVarbinds, 600);
		
		viewer.setLabelProvider(new SnmpTrapMonitorLabelProvider());
		setFilter(new SnmpTrapMonitorFilter());
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
		return "SnmpTrapMonitor"; //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(final SessionNotification n)
	{
		if (n.getCode() == SessionNotification.NEW_SNMP_TRAP)
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
}
