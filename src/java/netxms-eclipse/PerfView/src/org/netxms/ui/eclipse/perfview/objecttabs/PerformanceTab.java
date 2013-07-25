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
package org.netxms.ui.eclipse.perfview.objecttabs;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.PerfTabDci;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;
import org.netxms.ui.eclipse.perfview.PerfTabGraphSettings;
import org.netxms.ui.eclipse.perfview.objecttabs.internal.PerfTabGraph;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.AnimatedImage;

/**
 * Performance tab
 */
public class PerformanceTab extends ObjectTab
{
	private Map<Long, PerfTabGraph> charts = new HashMap<Long, PerfTabGraph>();
	private ScrolledComposite scroller;
	private Composite chartArea;
	private AnimatedImage waitingImage = null;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
		scroller = new ScrolledComposite(parent, SWT.V_SCROLL);
		
		chartArea = new Composite(scroller, SWT.NONE);
		chartArea.setBackground(SharedColors.getColor(SharedColors.OBJECT_TAB_BACKGROUND, parent.getDisplay()));
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		layout.marginWidth = 15;
		layout.marginHeight = 15;
		layout.horizontalSpacing = 10;
		layout.verticalSpacing = 10;
		chartArea.setLayout(layout);

		scroller.setContent(chartArea);
		scroller.setExpandVertical(true);
		scroller.setExpandHorizontal(true);
		scroller.getVerticalBar().setIncrement(20);
		scroller.addControlListener(new ControlAdapter() {
			public void controlResized(ControlEvent e)
			{
				Rectangle r = scroller.getClientArea();
				scroller.setMinSize(chartArea.computeSize(r.width, SWT.DEFAULT));
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public void objectChanged(final AbstractObject object)
	{
		for(PerfTabGraph chart : charts.values())
			chart.dispose();
		charts.clear();
		
		if (waitingImage != null)
			waitingImage.dispose();
		waitingImage = new AnimatedImage(chartArea, SWT.NONE);
		waitingImage.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, true, 2, 1));
		try
		{
			waitingImage.setImage(new URL("platform:/plugin/org.netxms.ui.eclipse.library/icons/loading.gif"));
		}
		catch(MalformedURLException e)
		{
		}
		updateChartAreaLayout();
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		Job job = new Job("Update performance tab") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				try
				{
					final PerfTabDci[] items = session.getPerfTabItems(object.getObjectId());
					new UIJob(PerformanceTab.this.getClientArea().getDisplay(), "Update performance tab") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							if (!getClientArea().isDisposed() &&
							    (PerformanceTab.this.getObject() != null) &&
							    (PerformanceTab.this.getObject().getObjectId() == object.getObjectId()))
							{
								update(items);
							}
							return Status.OK_STATUS;
						}
					}.schedule();
				}
				catch(Exception e)
				{
				}
				return Status.OK_STATUS;
			}
		};
		job.setSystem(true);
		job.schedule();
	}
	
	/**
	 * Update tab with received DCIs
	 * 
	 * @param items Performance tab items
	 */
	private void update(final PerfTabDci[] items)
	{
		if (waitingImage != null)
		{
			waitingImage.dispose();
			waitingImage = null;
		}

		for(PerfTabGraph chart : charts.values())
			chart.dispose();
		charts.clear();
		
		List<PerfTabGraphSettings> settings = new ArrayList<PerfTabGraphSettings>(items.length);
		for(int i = 0; i < items.length; i++)
		{
			try
			{
				PerfTabGraphSettings s = PerfTabGraphSettings.createFromXml(items[i].getPerfTabSettings());
				if (s.isEnabled())
				{
					s.setRuntimeDciInfo(items[i]);
					settings.add(s);
				}
			}
			catch(Exception e)
			{
			}
		}
		
		// Parent DCI ID can be template DCI ID. Replace it with real DCI ID.
		for(PerfTabGraphSettings s : settings)
			s.fixParentDciId(settings);
		
		// Sort DCIs: put top-level first, then by order number, then alphabetically
		Collections.sort(settings, new Comparator<PerfTabGraphSettings>() {
			@Override
			public int compare(PerfTabGraphSettings o1, PerfTabGraphSettings o2)
			{
				int result = Long.signum(o1.getParentDciId() - o2.getParentDciId());
				if (result == 0)
					result = Integer.signum(o1.getOrder() - o2.getOrder());
				if (result == 0)
				{
					// Sort top-level DCI's by chart title, and attached DCIs by legend name  
					if (o1.getParentDciId() == 0)
						result = o1.getRuntimeTitle().compareToIgnoreCase(o2.getRuntimeTitle());
					else
						result = o1.getRuntimeName().compareToIgnoreCase(o2.getRuntimeName());
				}
				return result;
			}
		});
		
		for(PerfTabGraphSettings s : settings)
		{
			if (s.getParentDciId() == 0)
			{
				PerfTabGraph chart = new PerfTabGraph(chartArea, getObject().getObjectId(), s.getRuntimeDciInfo(), s);
				charts.put(s.getRuntimeDciInfo().getId(), chart);
				
				final GridData gd = new GridData();
				gd.horizontalAlignment = SWT.FILL;
				gd.grabExcessHorizontalSpace = true;
				gd.heightHint = 250;
				chart.setLayoutData(gd);
			}
			else
			{
				PerfTabGraph chart = charts.get(s.getParentDciId());
				if (chart != null)
				{
					chart.addItem(s.getRuntimeDciInfo(), s);
				}
			}
		}
		
		for(PerfTabGraph chart : charts.values())
			chart.start();
		
		updateChartAreaLayout();
	}

	/**
	 * Update entire chart area layout after content change
	 */
	private void updateChartAreaLayout()
	{
		chartArea.layout();
		Rectangle r = scroller.getClientArea();
		scroller.setMinSize(chartArea.computeSize(r.width, SWT.DEFAULT));
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean showForObject(AbstractObject object)
	{
		return object instanceof AbstractNode;
	}
}
