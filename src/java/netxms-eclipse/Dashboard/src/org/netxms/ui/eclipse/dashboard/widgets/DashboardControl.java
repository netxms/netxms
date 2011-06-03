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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementLayout;

/**
 * Dashboard rendering control
 */
public class DashboardControl extends Composite
{
	private Dashboard dashboard;
	private boolean embedded = false;
	
	/**
	 * @param parent
	 * @param style
	 */
	public DashboardControl(Composite parent, int style, Dashboard dashboard, boolean embedded)
	{
		super(parent, style);
		this.dashboard = dashboard;
		this.embedded = embedded;
		setBackground(new Color(getDisplay(), 255, 255, 255));
		createContent();
	}

	/**
	 * Create dashboard's content
	 */
	private void createContent()
	{
		GridLayout layout = new GridLayout();
		layout.numColumns = dashboard.getNumColumns();
		layout.marginWidth = embedded ? 0 : 15;
		layout.marginHeight = embedded ? 0 : 15;
		layout.horizontalSpacing = 10;
		layout.verticalSpacing = 10;
		setLayout(layout);
		
		for(final DashboardElement e : dashboard.getElements())
		{
			final ElementWidget w = createElementWidget(e);
			final DashboardElementLayout el = w.getElementLayout();
			final GridData gd = new GridData();
			gd.grabExcessHorizontalSpace = (el.horizontalAlignment == DashboardElement.FILL);
			gd.horizontalAlignment = mapHorizontalAlignment(el.horizontalAlignment);
			gd.grabExcessVerticalSpace = (el.vertcalAlignment == DashboardElement.FILL);
			gd.verticalAlignment = mapVerticalAlignment(el.vertcalAlignment);
			gd.horizontalSpan = el.horizontalSpan;
			gd.verticalSpan = el.verticalSpan;
			w.setLayoutData(gd);
		}
	}
	
	/**
	 * Map dashboard element alignment info to SWT
	 * 
	 * @param a
	 * @return
	 */
	static int mapHorizontalAlignment(int a)
	{
		switch(a)
		{
			case DashboardElement.FILL:
				return SWT.FILL;
			case DashboardElement.LEFT:
				return SWT.LEFT;
			case DashboardElement.RIGHT:
				return SWT.RIGHT;
			case DashboardElement.CENTER:
				return SWT.CENTER;
		}
		return 0;
	}

	/**
	 * Map dashboard element alignment info to SWT
	 * 
	 * @param a
	 * @return
	 */
	static int mapVerticalAlignment(int a)
	{
		switch(a)
		{
			case DashboardElement.FILL:
				return SWT.FILL;
			case DashboardElement.TOP:
				return SWT.TOP;
			case DashboardElement.BOTTOM:
				return SWT.BOTTOM;
			case DashboardElement.CENTER:
				return SWT.CENTER;
		}
		return 0;
	}

	/**
	 * Factory method for creating dashboard elements
	 * 
	 * @param e
	 */
	private ElementWidget createElementWidget(DashboardElement e)
	{
		switch(e.getType())
		{
			case DashboardElement.LINE_CHART:
				return new LineChartElement(this, e.getData(), e.getLayout());
			case DashboardElement.BAR_CHART:
				return new BarChartElement(this, e.getData(), e.getLayout());
			case DashboardElement.PIE_CHART:
				return new PieChartElement(this, e.getData(), e.getLayout());
			case DashboardElement.STATUS_CHART:
				return new ObjectStatusChartElement(this, e.getData(), e.getLayout());
			case DashboardElement.LABEL:
				return new LabelElement(this, e.getData(), e.getLayout());
			case DashboardElement.DASHBOARD:
				return new EmbeddedDashboardElement(this, e.getData(), e.getLayout());
			case DashboardElement.NETWORK_MAP:
				return new NetworkMapElement(this, e.getData(), e.getLayout());
			case DashboardElement.GEO_MAP:
				return new GeoMapElement(this, e.getData(), e.getLayout());
			case DashboardElement.STATUS_INDICATOR:
				return new StatusIndicatorElement(this, e.getData(), e.getLayout());
			case DashboardElement.CUSTOM:
				return new CustomWidgetElement(this, e.getData(), e.getLayout());
			default:
				return new ElementWidget(this, e.getData(), e.getLayout());
		}
	}
}
