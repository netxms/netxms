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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Widget;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.DciData;
import org.netxms.ui.eclipse.charts.api.DataComparisonChart;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardDciInfo;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Base class for data comparison charts - like bar chart, pie chart, etc.
 */
public abstract class ComparisonChartElement extends ElementWidget
{
	protected DataComparisonChart chart;
	protected NXCSession session;

	private Runnable refreshTimer;
	private boolean updateInProgress = false;

	/**
	 * @param parent
	 * @param data
	 */
	public ComparisonChartElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);
		session = (NXCSession)ConsoleSharedData.getSession();

		setLayout(new FillLayout());	
	}
	
	/**
	 * Start refresh timer
	 */
	protected void startRefreshTimer()
	{
		final Display display = getDisplay();
		refreshTimer = new Runnable() {
			@Override
			public void run()
			{
				if (ComparisonChartElement.this.isDisposed())
					return;
				
				refreshData(getDciList());
				display.timerExec(30000, this);
			}
		};
		display.timerExec(30000, refreshTimer);
		refreshData(getDciList());
	}

	/**
	 * Refresh graph's data
	 */
	protected void refreshData(final DashboardDciInfo[] dciList)
	{
		if (updateInProgress)
			return;
		
		updateInProgress = true;
		
		ConsoleJob job = new ConsoleJob("Get DCI values for history graph", viewPart, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(int i = 0; i < dciList.length; i++)
				{
					final DciData data = session.getCollectedData(dciList[i].nodeId, dciList[i].dciId, null, null, 1);
					final int index = i;
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							if (!((Widget)chart).isDisposed())
							{
								chart.updateParameter(index, data.getLastValue().getValueAsDouble(), false);
							}
						}
					});
				}
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if (!((Widget)chart).isDisposed())
							chart.refresh();
						updateInProgress = false;
					}
				});
			}
	
			@Override
			protected String getErrorMessage()
			{
				return "Cannot get DCI values for comparision chart";
			}
	
			@Override
			protected void jobFailureHandler()
			{
				updateInProgress = false;
				super.jobFailureHandler();
			}
		};
		job.setUser(false);
		job.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		Point size = super.computeSize(wHint, hHint, changed);
		if ((hHint == SWT.DEFAULT) && (size.y < 250))
			size.y = 250;
		return size;
	}

	protected abstract DashboardDciInfo[] getDciList();
}
