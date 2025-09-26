/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.widgets;

import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Point;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.xnap.commons.i18n.I18n;
import com.google.gson.Gson;

/**
 * Base class for data comparison charts - like bar chart, pie chart, etc. with data obtained from server-side script.
 */
public abstract class ScriptedComparisonChartElement extends ElementWidget
{
   private final I18n i18n = LocalizationHelper.getI18n(ScriptedComparisonChartElement.class);

   protected Chart chart;
	protected NXCSession session;
   protected String script = null;
   protected long objectId = 0;
	protected int refreshInterval = 30;

	private ViewRefreshController refreshController;
	private boolean updateInProgress = false;

	/**
	 * @param parent
	 * @param data
	 */
   public ScriptedComparisonChartElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
   {
      super(parent, element, view);
      session = Registry.getSession();

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
      refreshController = new ViewRefreshController(view, refreshInterval, new Runnable() {
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

      Job job = new Job(i18n.tr("Reading DCI values for chart"), view, this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
            long contextObjectId = objectId;
            if (contextObjectId == 0)
               contextObjectId = getDashboardObjectId();
            else if (contextObjectId == AbstractObject.CONTEXT)
               contextObjectId = getContextObjectId();
            final Map<String, String> values = session.executeScriptedComparisonChartElement(getDashboardObjectId(), element.getIndex(), contextObjectId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (chart.isDisposed())
                     return;

                  chart.removeAllParameters();
                  for(Entry<String, String> e : values.entrySet())
                  {
                     ChartDciConfig item = new ChartDciConfig();
                     double value;
                     if (e.getValue().startsWith("{")) // Assume JSON format if value starts with {
                     {
                        GraphElement ge = new Gson().fromJson(e.getValue(), GraphElement.class);
                        String name = (ge.name != null) && !ge.name.isBlank() ? ge.name : e.getKey();
                        item.name = name;
                        item.dciDescription = name;
                        if ((ge.color != null) && !ge.color.isBlank())
                           item.setColor(ColorConverter.rgbToInt(ColorConverter.parseColorDefinition(ge.color)));
                        value = ge.value;
                     }
                     else
                     {
                        item.name = e.getKey();
                        item.dciDescription = item.name;
                        value = safeParseDouble(e.getValue());
                     }
                     int index = chart.addParameter(item);
                     chart.updateParameter(index, value, false);
                  }
                  chart.rebuild();
                  clearMessages();
                  updateInProgress = false;
               }
            });
			}

         /**
          * @see org.netxms.nxmc.base.jobs.Job#jobFailureHandler(java.lang.Exception)
          */
         @Override
         protected void jobFailureHandler(Exception e)
         {
            super.jobFailureHandler(e);
            updateInProgress = false;
         }

         @Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot read data for scripted chart");
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
