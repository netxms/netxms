/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.charts.views;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.util.ArrayList;
import java.util.Date;
import org.eclipse.jface.action.Action;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.charts.widgets.BarChart;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.RefreshAction;
import org.swtchart.Chart;

/**
 * Bar chart view for DCIs
 *
 */
public class LastValuesChart extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.charts.views.LastValuesChart";
	
	private NXCSession session;
	private ArrayList<GraphItem> items = new ArrayList<GraphItem>(8);
	private Chart chart;				// Current chart
	private BarChart barChart;
	private boolean updateInProgress = false;
	private Runnable refreshTimer;
	private boolean autoRefreshEnabled = true;
	private int autoRefreshInterval = 30000;	// 30 seconds

	private RefreshAction actionRefresh;
	private Action actionAutoRefresh;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);

		final Display display = site.getShell().getDisplay();
		refreshTimer = new Runnable() {
			@Override
			public void run()
			{
				if (chart.isDisposed())
					return;
				
				updateChart();
				display.timerExec(autoRefreshInterval, this);
			}
		};
		
		session = NXMCSharedData.getInstance().getSession();
		
		// Extract DCI ids from view id
		// (first field will be unique view id, so we skip it)
		String id = site.getSecondaryId();
		String[] fields = id.split("&");
		if (!fields[0].equals(HistoryGraph.PREDEFINED_GRAPH_SUBID))
		{
			for(int i = 1; i < fields.length; i++)
			{
				String[] subfields = fields[i].split("\\@");
				if (subfields.length == 6)
				{
					try
					{
						items.add(new GraphItem(
								Long.parseLong(subfields[0], 10),	// Node ID 
								Long.parseLong(subfields[1], 10),	// DCI ID
								Integer.parseInt(subfields[2], 10),	// source
								Integer.parseInt(subfields[3], 10),	// data type
								URLDecoder.decode(subfields[4], "UTF-8"),   // name
								URLDecoder.decode(subfields[5], "UTF-8"))); // description
					}
					catch(NumberFormatException e)
					{
						e.printStackTrace();
					}
					catch(UnsupportedEncodingException e)
					{
						e.printStackTrace();
					}
				}
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		barChart = new BarChart(parent, SWT.NONE);
		chart = barChart;

		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		getDataFromServer();
		if (autoRefreshEnabled)
			getSite().getShell().getDisplay().timerExec(autoRefreshInterval, refreshTimer);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		chart.setFocus();
	}

}
