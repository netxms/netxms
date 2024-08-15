/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.views;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.PerfTabDci;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.xml.XMLTools;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.views.helpers.PerfViewGraphSettings;
import org.netxms.nxmc.modules.datacollection.widgets.PerfTabGraph;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Performance view
 */
public class PerformanceView extends ObjectView
{
   private static Logger logger = LoggerFactory.getLogger(PerformanceView.class);
   private I18n i18n = LocalizationHelper.getI18n(PerformanceView.class);

   private Map<String, PerfTabGraph> charts = new HashMap<String, PerfTabGraph>();
   private ScrolledComposite scroller;
   private Composite chartArea;

   /**
    * @param name
    * @param image
    */
   public PerformanceView()
   {
      super(LocalizationHelper.getI18n(PerformanceView.class).tr("Performance"), ResourceManager.getImageDescriptor("icons/object-views/performance.png"), "objects.performance", false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      scroller = new ScrolledComposite(parent, SWT.V_SCROLL);

      chartArea = new Composite(scroller, SWT.NONE);
      chartArea.setBackground(ThemeEngine.getBackgroundColor("Dashboard"));

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
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            Rectangle r = scroller.getClientArea();
            scroller.setMinSize(chartArea.computeSize(r.width, SWT.DEFAULT));
         }
      });
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof DataCollectionTarget);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      for(PerfTabGraph chart : charts.values())
         chart.dispose();
      charts.clear();

      if (object == null)
         return;

      updateChartAreaLayout();

      Job job = new Job(i18n.tr("Updating performance view"), this) {
         @Override
         protected void run(IProgressMonitor monitor)
         {
            try
            {
               final List<PerfTabDci> items = session.getPerfTabItems(object.getObjectId());
               runInUIThread(() -> {
                  if (!getViewArea().isDisposed() && (PerformanceView.this.getObject() != null) && (PerformanceView.this.getObject().getObjectId() == object.getObjectId()))
                  {
                     update(items);
                  }
               });
            }
            catch(Exception e)
            {
               logger.error("Exception in performance tab loading job", e);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot update performance view";
         }
      };
      job.setSystem(true);
      job.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      Job job = new Job(i18n.tr("Updating performance view"), this) {
         @Override
         protected void run(IProgressMonitor monitor)
         {
            try
            {
               final List<PerfTabDci> items = session.getPerfTabItems(getObjectId());
               runInUIThread(() -> {
                  if (!getViewArea().isDisposed() && (PerformanceView.this.getObject() != null) && (PerformanceView.this.getObject().getObjectId() == getObjectId()))
                  {
                     update(items);
                  }
               });
            }
            catch(Exception e)
            {
               logger.error("Exception in performance tab loading job", e);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot update performance view";
         }
      };
      job.setSystem(true);
      job.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 40;
   }

   /**
    * Update tab with received DCIs
    * 
    * @param items Performance tab items
    */
   private void update(final List<PerfTabDci> items)
   {
      for(PerfTabGraph chart : charts.values())
         chart.dispose();
      charts.clear();

      List<PerfViewGraphSettings> settings = new ArrayList<PerfViewGraphSettings>(items.size());
      for(PerfTabDci dci : items)
      {
         try
         {
            PerfViewGraphSettings s = XMLTools.createFromXml(PerfViewGraphSettings.class, dci.getPerfTabSettings());
            if (s.isEnabled())
            {
               s.setRuntimeDciInfo(dci);
               settings.add(s);
            }
         }
         catch(Exception e)
         {
         }
      }

      // Sort DCIs: by group name, then by order number, then alphabetically
      Collections.sort(settings, new Comparator<PerfViewGraphSettings>() {
         @Override
         public int compare(PerfViewGraphSettings o1, PerfViewGraphSettings o2)
         {
            int result = Integer.signum(o1.getOrder() - o2.getOrder());
            if (result == 0)
               result = o1.getGroupName().compareToIgnoreCase(o2.getGroupName());
            if (result == 0)
            {
               // Sort top-level DCI's by chart title, and attached DCIs by legend name
               if (o1.getGroupName().isEmpty())
                  result = o1.getRuntimeTitle().compareToIgnoreCase(o2.getRuntimeTitle());
               else
                  result = o1.getRuntimeName().compareToIgnoreCase(o2.getRuntimeName());
            }
            return result;
         }
      });

      for(PerfViewGraphSettings s : settings)
      {
         String groupName = s.getGroupName();
         PerfTabGraph chart = groupName.isEmpty() ? null : charts.get(groupName);
         if (chart == null)
         {
            chart = new PerfTabGraph(chartArea, getObject().getObjectId(), s.getRuntimeDciInfo(), s, this, () -> isVisible());
            charts.put(groupName.isEmpty() ? "##" + Long.toString(s.getRuntimeDciInfo().getId()) : groupName, chart);

            final GridData gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.verticalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            gd.heightHint = 320;
            chart.setLayoutData(gd);
         }
         else
         {
            chart.addItem(s.getRuntimeDciInfo(), s);
            if (chart.getChart().hasExtendedLegend())
               ((GridData)chart.getLayoutData()).heightHint += 15;
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
}
