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

import java.util.ArrayList;
import java.util.List;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.internal.dialogs.PropertyDialog;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementLayout;

/**
 * Dashboard rendering control
 */
public class DashboardControl extends Composite
{
	private Dashboard dashboard;
	private List<DashboardElement> elements;
	private boolean embedded = false;
	private boolean editMode = false;
	private boolean modified = false;
	
	/**
	 * @param parent
	 * @param style
	 */
	public DashboardControl(Composite parent, int style, Dashboard dashboard, boolean embedded)
	{
		super(parent, style);
		this.dashboard = dashboard;
		this.embedded = embedded;
		elements = new ArrayList<DashboardElement>(dashboard.getElements());
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
			createElementWidget(e);
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
		ElementWidget w;
		switch(e.getType())
		{
			case DashboardElement.LINE_CHART:
				w = new LineChartElement(this, e.getData(), e.getLayout());
				break;
			case DashboardElement.BAR_CHART:
				w = new BarChartElement(this, e.getData(), e.getLayout());
				break;
			case DashboardElement.PIE_CHART:
				w = new PieChartElement(this, e.getData(), e.getLayout());
				break;
			case DashboardElement.STATUS_CHART:
				w = new ObjectStatusChartElement(this, e.getData(), e.getLayout());
				break;
			case DashboardElement.LABEL:
				w = new LabelElement(this, e.getData(), e.getLayout());
				break;
			case DashboardElement.DASHBOARD:
				w = new EmbeddedDashboardElement(this, e.getData(), e.getLayout());
				break;
			case DashboardElement.NETWORK_MAP:
				w = new NetworkMapElement(this, e.getData(), e.getLayout());
				break;
			case DashboardElement.GEO_MAP:
				w = new GeoMapElement(this, e.getData(), e.getLayout());
				break;
			case DashboardElement.STATUS_INDICATOR:
				w = new StatusIndicatorElement(this, e.getData(), e.getLayout());
				break;
			case DashboardElement.CUSTOM:
				w = new CustomWidgetElement(this, e.getData(), e.getLayout());
				break;
			default:
				w = new ElementWidget(this, e.getData(), e.getLayout());
				break;
		}

		final DashboardElementLayout el = w.getElementLayout();
		final GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = (el.horizontalAlignment == DashboardElement.FILL);
		gd.horizontalAlignment = mapHorizontalAlignment(el.horizontalAlignment);
		gd.grabExcessVerticalSpace = (el.vertcalAlignment == DashboardElement.FILL);
		gd.verticalAlignment = mapVerticalAlignment(el.vertcalAlignment);
		gd.horizontalSpan = el.horizontalSpan;
		gd.verticalSpan = el.verticalSpan;
		w.setLayoutData(gd);
		
		return w;
	}

	/**
	 * @return the editMode
	 */
	public boolean isEditMode()
	{
		return editMode;
	}

	/**
	 * @param editMode the editMode to set
	 */
	public void setEditMode(boolean editMode)
	{
		this.editMode = editMode;
		for(Control c : getChildren())
		{
			if (c instanceof ElementWidget)
			{
				((ElementWidget)c).setEditMode(editMode);
			}
		}
	}
	
	/**
	 * @param element
	 */
	@SuppressWarnings("restriction")
	private void addElement(DashboardElement element)
	{
		PropertyDialog dlg = PropertyDialog.createDialogOn(getShell(), null, element);
		if (dlg.open() == Window.CANCEL)
			return;	// element creation cancelled
		
		elements.add(element);
		createElementWidget(element);
		layout(true, true);
		modified = true;
	}
	
	/**
	 * Add alarm browser widget to dashboard
	 */
	public void addAlarmBrowser()
	{
		DashboardElement e = new DashboardElement(DashboardElement.LABEL, "<element>\n\t<title>Label</title>\n</element>");
		addElement(e);
	}
	
	/**
	 * Add label widget to dashboard
	 */
	public void addLabel()
	{
		DashboardElement e = new DashboardElement(DashboardElement.LABEL, "<element>\n\t<title>Label</title>\n</element>");
		addElement(e);
	}
	
	/**
	 * Add pie chart widget to dashboard
	 */
	public void addPieChart()
	{
		DashboardElement e = new DashboardElement(DashboardElement.PIE_CHART, "<element>\n\t<showIn3D>true</showIn3D>\n\t<dciList length=\"0\">\n\t</dciList>\n</element>");
		addElement(e);
	}
}
