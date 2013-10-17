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
package org.netxms.ui.eclipse.snmp.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.widgets.SnmpTrapTraceWidget;
import org.netxms.ui.eclipse.views.AbstractTraceView;
import org.netxms.ui.eclipse.widgets.AbstractTraceWidget;

/**
 * SNMP trap monitor
 */
public class SnmpTrapMonitor extends AbstractTraceView
{
	public static final String ID = "org.netxms.ui.eclipse.snmp.views.SnmpTrapMonitor"; //$NON-NLS-1$
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
	 */
	@Override
	public void init(IViewSite site, IMemento memento) throws PartInitException
	{
		super.init(site, memento);
		if (memento != null)
		{
			final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
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
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
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

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.views.AbstractTraceView#createTraceWidget(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected AbstractTraceWidget createTraceWidget(Composite parent)
	{
		return new SnmpTrapTraceWidget(parent, SWT.NONE, this);
	}
}
