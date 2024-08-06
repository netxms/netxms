/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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

import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.charts.widgets.Chart;
import org.netxms.ui.eclipse.compatibility.GraphItem;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.ViewRefreshController;
import com.google.gson.Gson;

/**
 * Base class for data comparison charts - like bar chart, pie chart, etc. with data obtained from server-side script.
 */
public abstract class ScriptedComparisonChartElement extends ElementWidget
{
   protected Chart chart;
	protected NXCSession session;
   protected String script = null;
   protected long objectId = 0;
	protected int refreshInterval = 30;
	protected boolean updateThresholds = false;

	private ViewRefreshController refreshController;
	private boolean updateInProgress = false;

	/**
	 * @param parent
	 * @param data
	 */
   public ScriptedComparisonChartElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
   {
      super(parent, element, viewPart);
      session = ConsoleSharedData.getSession();

		addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (refreshController != null)
               refreshController.dispose();
         }
      });
	}

	/**
	 * Start refresh timer
	 */
	protected void startRefreshTimer()
	{
		refreshController = new ViewRefreshController(viewPart, refreshInterval, new Runnable() {
			@Override
			public void run()
			{
				if (ScriptedComparisonChartElement.this.isDisposed())
					return;

            refreshData();
			}
		});
      refreshData();
	}

	/**
	 * Refresh graph's data
	 */
   protected void refreshData()
	{
		if (updateInProgress)
			return;

		updateInProgress = true;

		ConsoleJob job = new ConsoleJob(Messages.get().ComparisonChartElement_JobTitle, viewPart, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
            long contextObjectId = objectId;
            if (contextObjectId == 0)
               contextObjectId = getDashboardObjectId();
            else if (contextObjectId == AbstractObject.CONTEXT)
               contextObjectId = getContextObjectId();
            final Map<String, String> values = session.queryScript(contextObjectId, script, null, null);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (chart.isDisposed())
                     return;

                  chart.removeAllParameters();
                  for(Entry<String, String> e : values.entrySet())
                  {
                     GraphItem item;
                     double value;
                     if (e.getValue().startsWith("{")) // Assume JSON format if value starts with {
                     {
                        GraphElement ge = new Gson().fromJson(e.getValue(), GraphElement.class);
                        String name = (ge.name != null) && !ge.name.isBlank() ? ge.name : e.getKey();
                        item = new GraphItem(name, name, null);
                        if ((ge.color != null) && !ge.color.isBlank())
                           item.setColor(ColorConverter.rgbToInt(ColorConverter.parseColorDefinition(ge.color)));
                        value = ge.value;
                     }
                     else
                     {
                        item = new GraphItem(e.getKey(), e.getKey(), null);
                        value = safeParseDouble(e.getValue());
                     }
                     int index = chart.addParameter(item);
                     chart.updateParameter(index, value, false);
                  }
                  chart.rebuild();
                  chart.clearErrors();
						updateInProgress = false;
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().ComparisonChartElement_JobError;
			}

			@Override
			protected void jobFailureHandler()
			{
				updateInProgress = false;
				super.jobFailureHandler();
			}

			@Override
			protected IStatus createFailureStatus(final Exception e)
			{
            Activator.logError("Cannot read data for scripted chart", e);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						chart.addError(getErrorMessage() + " (" + e.getLocalizedMessage() + ")"); //$NON-NLS-1$ //$NON-NLS-2$
					}
				});
				return Status.OK_STATUS;
			}
		};
		job.setUser(false);
		job.start();
	}

   /**
    * Parse double value without throwing an exception.
    *
    * @param s string to parse
    * @return parsed double value or 0
    */
   private static double safeParseDouble(String s)
   {
      try
      {
         return Double.parseDouble(s);
      }
      catch(NumberFormatException e)
      {
         return 0;
      }
   }

   /**
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

   private static class GraphElement
   {
      String name = null;
      String color = null;
      double value = 0;
   }
}
