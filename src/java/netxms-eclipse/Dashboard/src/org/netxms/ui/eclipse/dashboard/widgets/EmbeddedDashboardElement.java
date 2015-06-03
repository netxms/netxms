/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.dashboard.widgets.internal.EmbeddedDashboardConfig;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ViewRefreshController;

/**
 * Embedded dashboard element
 *
 */
public class EmbeddedDashboardElement extends ElementWidget
{
	private Dashboard[] objects;
	private EmbeddedDashboardConfig config;
	private DashboardControl control = null;
	private int current = -1;
	
	/**
	 * @param parent
	 * @param data
	 */
	public EmbeddedDashboardElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, SWT.NONE, element, viewPart);

		try
		{
			config = EmbeddedDashboardConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new EmbeddedDashboardConfig();
		}
		
		NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		objects = new Dashboard[config.getDashboardObjects().length];
		for(int i = 0; i < objects.length; i++)
			objects[i] = (Dashboard)session.findObjectById(config.getDashboardObjects()[i], Dashboard.class);
		
		FillLayout layout = new FillLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		setLayout(layout);
		
		if (objects.length > 1)
		{
			nextDashboard();
			
			final ViewRefreshController refreshControler = new ViewRefreshController(viewPart, config.getDisplayInterval(), new Runnable() {
            @Override
            public void run()
            {
               if (!isDisposed())
                  nextDashboard();
            }
         });
			
			addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               refreshControler.dispose();
            }
         });
		}
		else
		{
			if ((objects != null) && (objects.length > 0) && (objects[0] != null))
				new DashboardControl(this, SWT.NONE, objects[0], viewPart, getSelectionProvider(), true);
		}
	}
	
	/**
	 * Show next dashboard in chain
	 */
	private void nextDashboard()
	{
		if (control != null)
			control.dispose();
		current++;
		if (current >= objects.length)
			current = 0;
		if (objects[current] != null)
			control = new DashboardControl(this, SWT.NONE, objects[current], viewPart, getSelectionProvider(), true);	/* TODO: set embedded=false if border=true */
		else
			control = null;
		getParent().layout(true, true);
	}
}
