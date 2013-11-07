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
package org.netxms.ui.eclipse.eventmanager.views;

import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.eventmanager.widgets.EventTraceWidget;
import org.netxms.ui.eclipse.views.AbstractTraceView;
import org.netxms.ui.eclipse.widgets.AbstractTraceWidget;

/**
 * Event monitor
 */
public class EventMonitor extends AbstractTraceView
{
	public static final String ID = "org.netxms.ui.eclipse.eventmanager.views.EventMonitor"; //$NON-NLS-1$
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.views.AbstractTraceView#fillLocalPullDown(org.eclipse.jface.action.IMenuManager)
	 */
	@Override
	protected void fillLocalPullDown(IMenuManager manager)
	{
		super.fillLocalPullDown(manager);
		manager.add(new Separator());
		manager.add(((EventTraceWidget)getTraceWidget()).getActionShowColor());
		manager.add(((EventTraceWidget)getTraceWidget()).getActionShowIcons());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.views.AbstractTraceView#createTraceWidget(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected AbstractTraceWidget createTraceWidget(Composite parent)
	{
		return new EventTraceWidget(parent, SWT.NONE, this);
	}
}
