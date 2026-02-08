/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.modules.dashboards.widgets.helpers.UnmappedDciException;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.xnap.commons.i18n.I18n;

/**
 * Base class for data comparison charts - like bar chart, pie chart, etc.
 */
public abstract class ComparisonChartElement extends ElementWidget
{
   private final I18n i18n = LocalizationHelper.getI18n(ComparisonChartElement.class);

   protected Chart chart;
	protected NXCSession session;
	protected int refreshInterval = 30;
   protected List<ChartDciConfig> runtimeDciList = new ArrayList<>();

	private ViewRefreshController refreshController;
	private boolean updateInProgress = false;

   /**
    * @param parent parent composite
    * @param element dashboard element
    * @param view owning view
    */
   public ComparisonChartElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
   {
      super(parent, element, view);
      session = Registry.getSession();

      addDisposeListener((e) -> {
         if (refreshController != null)
            refreshController.dispose();
      });
   }

   /**
    * Configure metrics on chart and start refresh timer on success
    */
   protected void configureMetrics()
   {
      Job job = new Job("Reading measurement unit information", view, this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            DciValue[] nodeDciList = null;
            for(ChartDciConfig dci : getDciList())
            {
               if ((dci.nodeId == 0) || (dci.nodeId == AbstractObject.CONTEXT))
               {
                  AbstractObject contextObject = getContext();
                  if (contextObject == null)
                     continue;

                  if (nodeDciList == null)
                     nodeDciList = session.getLastValues(contextObject.getObjectId());

                  if (dci.regexMatch)
                  {
                     try
                     {
                        Pattern namePattern = Pattern.compile(dci.dciName);
                        Pattern descriptionPattern = Pattern.compile(dci.dciDescription);
                        Pattern tagPattern = Pattern.compile(dci.dciTag);
                        for(DciValue dciInfo : nodeDciList)
                        {
                           if (dciInfo.getDcObjectType() != DataCollectionObject.DCO_TYPE_ITEM)
                              continue;

                           Matcher matcher = null;
                           boolean match = false;

                           if (!dci.dciName.isEmpty())
                           {
                              matcher = namePattern.matcher(dciInfo.getName());
                              match = matcher.find();
                           }
                           if (!match && !dci.dciDescription.isEmpty())
                           {
                              matcher = descriptionPattern.matcher(dciInfo.getDescription());
                              match = matcher.find();
                           }
                           if (!match && !dci.dciTag.isEmpty())
                           {
                              matcher = tagPattern.matcher(dciInfo.getUserTag());
                              match = matcher.find();
                           }

                           if (match)
                           {
                              ChartDciConfig instance = new ChartDciConfig(dci, matcher, dciInfo);
                              runtimeDciList.add(instance);
                              if (!dci.multiMatch)
                                 break;
                           }
                        }
                     }
                     catch(PatternSyntaxException e)
                     {
                        throw new Exception(i18n.tr("Error in DCI matching regular expression: ") + e.getLocalizedMessage(), e);
                     }
                  }
                  else
                  {
                     for(DciValue dciInfo : nodeDciList)
                     {
                        if (dciInfo.getDcObjectType() != DataCollectionObject.DCO_TYPE_ITEM)
                           continue;

                        if ((!dci.dciName.isEmpty() && dciInfo.getName().equalsIgnoreCase(dci.dciName)) ||
                            (!dci.dciDescription.isEmpty() && dciInfo.getDescription().equalsIgnoreCase(dci.dciDescription)) ||
                            (!dci.dciTag.isEmpty() && dciInfo.getUserTag().equalsIgnoreCase(dci.dciTag)))
                        {
                           runtimeDciList.add(new ChartDciConfig(dci, dciInfo));
                           if (!dci.multiMatch)
                              break;
                        }
                     }            
                  }
               }
               else
               {
                  runtimeDciList.add(dci);
               }
            }

            runInUIThread(() -> {
               if (chart.isDisposed())
                  return;

               for(ChartDciConfig dci : runtimeDciList)
               {
                  chart.addParameter(new ChartDciConfig(dci));
               }

               chart.rebuild();
               layout(true, true);
               startRefreshTimer();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read measurement unit information");
         }
      };
      job.setUser(false);
      job.start();
   }

	/**
	 * Start refresh timer
	 */
	protected void startRefreshTimer()
	{
      refreshController = new ViewRefreshController(view, refreshInterval, () -> {
         if (ComparisonChartElement.this.isDisposed())
            return;
         refreshData();
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

      final long dashboardId = getDashboardObjectId();
      Job job = new Job(i18n.tr("Reading DCI data for chart"), view, this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
            final DataSeries[] data = new DataSeries[runtimeDciList.size()];
            for(int i = 0; i < runtimeDciList.size(); i++)
				{
               ChartDciConfig dci = runtimeDciList.get(i);
               if ((dci.nodeId == AbstractObject.UNKNOWN) || (dci.dciId <= 0))
                  throw new UnmappedDciException();
               if (dci.type == ChartDciConfig.ITEM)
                  data[i] = session.getCollectedData(dci.nodeId, dci.dciId, null, null, 1, HistoricalDataType.PROCESSED, dashboardId);
					else
                  data[i] = session.getCollectedTableData(dci.nodeId, dci.dciId, dci.instance, dci.column, null, null, 1, dashboardId);
				}

            runInUIThread(() -> {
               updateInProgress = false;
               if (chart.isDisposed())
                  return;

               for(int i = 0; i < data.length; i++)
					{
                  chart.updateParameter(i, data[i], false);
					}
               chart.refresh();
               clearMessages();
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot read DCI data");
			}

         @Override
         protected void jobFailureHandler(Exception e)
         {
            runInUIThread(() -> {
               updateInProgress = false;
            });
         }
		};
		job.setUser(false);
		job.start();
	}

   /**
    * Get list of configured DCIs.
    *
    * @return list of configured DCIs
    */
	protected abstract ChartDciConfig[] getDciList();
}
