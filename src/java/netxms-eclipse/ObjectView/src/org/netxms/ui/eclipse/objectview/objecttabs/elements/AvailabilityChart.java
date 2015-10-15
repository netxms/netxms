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
package org.netxms.ui.eclipse.objectview.objecttabs.elements;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.ServiceContainer;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.ChartFactory;
import org.netxms.ui.eclipse.charts.api.DataComparisonChart;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * Availability chart for business services
 */
public class AvailabilityChart extends OverviewPageElement
{
	private DataComparisonChart dayChart;
	private DataComparisonChart weekChart;
	private DataComparisonChart monthChart;
	private ColorCache colors;
	
	/**
	 * @param parent
	 * @param object
	 * @param objectTab
	 */
	public AvailabilityChart(Composite parent, OverviewPageElement anchor, ObjectTab objectTab)
	{
		super(parent, anchor, objectTab);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#getTitle()
	 */
	@Override
	protected String getTitle()
	{
		return Messages.get().AvailabilityChart_Title;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#onObjectChange()
	 */
	@Override
	protected void onObjectChange()
	{
		ServiceContainer service = (ServiceContainer)getObject();
		
		dayChart.updateParameter(0, service.getUptimeForDay(), false);
		dayChart.updateParameter(1, 100.0 - service.getUptimeForDay(), true);

		weekChart.updateParameter(0, service.getUptimeForWeek(), false);
		weekChart.updateParameter(1, 100.0 - service.getUptimeForWeek(), true);
		
		monthChart.updateParameter(0, service.getUptimeForMonth(), false);
		monthChart.updateParameter(1, 100.0 - service.getUptimeForMonth(), true);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.DashboardElement#createClientArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createClientArea(Composite parent)
	{
		Composite clientArea = new Composite(parent, SWT.NONE);
		colors = new ColorCache(clientArea);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 4;
		clientArea.setLayout(layout);
		
		dayChart = createChart(clientArea, Messages.get().AvailabilityChart_Today);
		weekChart = createChart(clientArea, Messages.get().AvailabilityChart_ThisWeek);
		monthChart = createChart(clientArea, Messages.get().AvailabilityChart_ThisMonth);
		
		Canvas legend = new Canvas(clientArea, SWT.NONE);
		legend.addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e)
			{
				paintLegend(e.gc);
			}
		});
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		legend.setLayoutData(gd);
		
		return clientArea;
	}
	
	/**
	 * @param parent
	 * @param title
	 * @return
	 */
	private DataComparisonChart createChart(Composite parent, String title)
	{
		DataComparisonChart chart = ChartFactory.createPieChart(parent, SWT.NONE);
		chart.setTitleVisible(true);
		chart.set3DModeEnabled(true);
		chart.setChartTitle(title);
		chart.setLegendVisible(false);
		chart.setLabelsVisible(false);
		chart.setRotation(225.0);
		
		chart.addParameter(new GraphItem(0, 0, 0, 0, Messages.get().AvailabilityChart_Up, Messages.get().AvailabilityChart_Up), 100);
		chart.addParameter(new GraphItem(0, 0, 0, 0, Messages.get().AvailabilityChart_Down, Messages.get().AvailabilityChart_Down), 0);
		chart.setPaletteEntry(0, new ChartColor(127, 154, 72));
		chart.setPaletteEntry(1, new ChartColor(158, 65, 62));
		chart.initializationComplete();
		
		GridData gd = new GridData();
		gd.widthHint = 250;
		gd.heightHint = 190;
		((Control)chart).setLayoutData(gd);
		return chart;
	}
	
	/**
	 * Paint legend for charts
	 * 
	 * @param gc
	 */
	private void paintLegend(GC gc)
	{
		final int th = gc.textExtent(Messages.get().AvailabilityChart_Uptime + Messages.get().AvailabilityChart_Downtime).y;
		final Color fg = SharedColors.getColor(SharedColors.SERVICE_AVAILABILITY_LEGEND, getDisplay());
		
		gc.setBackground(colors.create(127, 154, 72));
		gc.setForeground(ColorConverter.adjustColor(gc.getBackground(), fg.getRGB(), 0.2f, colors));
		gc.fillRectangle(5, 10, th, th);
		gc.drawRectangle(5, 10, th, th);

		gc.setBackground(colors.create(158, 65, 62));
		gc.setForeground(ColorConverter.adjustColor(gc.getBackground(), fg.getRGB(), 0.2f, colors));
		gc.fillRectangle(5, 40, th, th);
		gc.drawRectangle(5, 40, th, th);

		gc.setForeground(fg);
		gc.drawText(Messages.get().AvailabilityChart_Uptime, 10 + th, 10, true);
		gc.drawText(Messages.get().AvailabilityChart_Downtime, 10 + th, 40, true);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean isApplicableForObject(AbstractObject object)
	{
		return object instanceof ServiceContainer;
	}
}
