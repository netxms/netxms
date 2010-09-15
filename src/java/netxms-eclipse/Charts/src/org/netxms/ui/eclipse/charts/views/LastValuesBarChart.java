/**
 * NetXMS - open source network management system Copyright (C) 2003-2010 Victor Kirhenshtein
 * 
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.charts.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.widgets.BarChart;
import org.swtchart.IBarSeries;

/**
 * Bar chart view for DCIs
 * 
 */
public class LastValuesBarChart extends LastValuesChart
{
	public static final String ID = "org.netxms.ui.eclipse.charts.views.LastValuesBarChart";

	private Action actionLogScale;
	private Action actionShowHorizontal;

	private Action actionShowVertical;
	private BarChart barChart;
	private boolean useLogScale = false;

	/**
	 * Add single DCI to chart
	 * 
	 * @param data
	 *           DCI data
	 */
	@Override
	protected void addItemToChart(int index, GraphItem item, double data, boolean padding)
	{
		IBarSeries series = barChart.addBarSeries(index, item.getDescription(), data);
		series.setBarMargin(25);
	}

	/**
	 * Fill action bars
	 */
	@Override
	protected void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Create actions
	 */
	@Override
	protected void createActions()
	{
		super.createActions();

		actionLogScale = new Action()
		{
			@Override
			public void run()
			{
				useLogScale = !useLogScale;
				setChecked(useLogScale);
				chart.getAxisSet().getYAxis(0).enableLogScale(useLogScale);
				chart.redraw();
			}
		};
		actionLogScale.setText("&Logarithmic scale");
		actionLogScale.setChecked(useLogScale);

		actionShowVertical = new Action()
		{
			@Override
			public void run()
			{
				setChecked(true);
				actionShowHorizontal.setChecked(false);
				barChart.setOrientation(SWT.HORIZONTAL);
				barChart.redraw();
			}
		};
		actionShowVertical.setText("Vertical");
		actionShowVertical.setImageDescriptor(Activator.getImageDescriptor("icons/bar_vertical.png"));
		actionShowVertical.setChecked(barChart.getOrientation() == SWT.HORIZONTAL);

		actionShowHorizontal = new Action()
		{
			@Override
			public void run()
			{
				barChart.setOrientation(SWT.VERTICAL);
				setChecked(true);
				actionShowVertical.setChecked(false);
				barChart.redraw();
			}
		};
		actionShowHorizontal.setText("Horizontal");
		actionShowHorizontal.setImageDescriptor(Activator.getImageDescriptor("icons/bar_horizontal.png"));
		actionShowHorizontal.setChecked(barChart.getOrientation() == SWT.VERTICAL);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		barChart = new BarChart(parent, SWT.NONE);
		barChart.setCategories(new String[] { "" });
		barChart.getAxisSet().getYAxis(0).enableLogScale(useLogScale);
		chart = barChart;

		initChart();
	}

	/**
	 * Fill context menu
	 * 
	 * @param manager
	 */
	@Override
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionShowVertical);
		manager.add(actionShowHorizontal);
		manager.add(new Separator());
		manager.add(actionAutoRefresh);
		manager.add(actionLogScale);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionShowVertical);
		manager.add(actionShowHorizontal);
		manager.add(new Separator());
		manager.add(actionAutoRefresh);
		manager.add(actionLogScale);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionShowVertical);
		manager.add(actionShowHorizontal);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}
}
