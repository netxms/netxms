/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.traffic.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.Table;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.ObservationPoint;
import org.netxms.client.objects.TrafficObserver;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.traffic.widgets.TrafficQueryTable;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * "Traffic" view for observation point objects: live stat header plus active hosts,
 * L7 breakdown, and top talkers tables served through the traffic connector.
 */
public class ObservationPointView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(ObservationPointView.class);

   private static final String[] STAT_METRICS = { "ThroughputIn", "ThroughputOut", "ActiveHosts", "ActiveFlows", "Drops", "TcpRetransmits" };

   private Label[] statLabels = new Label[STAT_METRICS.length];
   private TrafficQueryTable activeHostsTable;
   private TrafficQueryTable l7Table;
   private TrafficQueryTable topTalkersTable;

   /**
    * Create view
    */
   public ObservationPointView()
   {
      super(LocalizationHelper.getI18n(ObservationPointView.class).tr("Traffic"),
            ResourceManager.getImageDescriptor("icons/object-views/performance.png"), "objects.observation-point-traffic", false);
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
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 30;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context instanceof ObservationPoint);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      parent.setLayout(layout);

      final String[] statTitles = {
            i18n.tr("Throughput (in)"),
            i18n.tr("Throughput (out)"),
            i18n.tr("Active hosts"),
            i18n.tr("Active flows"),
            i18n.tr("Drops"),
            i18n.tr("TCP retransmits")
      };

      Composite header = new Composite(parent, SWT.NONE);
      header.setLayout(new GridLayout(STAT_METRICS.length, true));
      header.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
      for(int i = 0; i < STAT_METRICS.length; i++)
      {
         Composite cell = new Composite(header, SWT.NONE);
         cell.setLayout(new GridLayout());
         cell.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

         Label title = new Label(cell, SWT.CENTER);
         title.setText(statTitles[i]);
         title.setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, false));

         statLabels[i] = new Label(cell, SWT.CENTER);
         statLabels[i].setText("-");
         statLabels[i].setLayoutData(new GridData(SWT.CENTER, SWT.CENTER, true, false));
      }

      SashForm content = new SashForm(parent, SWT.HORIZONTAL);
      content.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      Group activeHostsGroup = new Group(content, SWT.NONE);
      activeHostsGroup.setText(i18n.tr("Active hosts"));
      activeHostsGroup.setLayout(new FillLayout());
      activeHostsTable = new TrafficQueryTable(activeHostsGroup, SWT.NONE, this, "ActiveHosts", i18n.tr("Reading active host list"),
            () -> (getObject() != null) ? session.getActiveTrafficHosts(getObjectId()) : null);

      Group l7Group = new Group(content, SWT.NONE);
      l7Group.setText(i18n.tr("Application breakdown"));
      l7Group.setLayout(new FillLayout());
      l7Table = new TrafficQueryTable(l7Group, SWT.NONE, this, "PointL7", i18n.tr("Reading application breakdown"),
            () -> queryPointTable("L7Breakdown", TrafficObserver.CAP_POINT_L7));
      l7Table.setNoDataMessage(i18n.tr("Not supported by traffic analyzer"));
      l7Table.setSortColumn("BYTES_RCVD", SWT.DOWN);

      Group topTalkersGroup = new Group(content, SWT.NONE);
      topTalkersGroup.setText(i18n.tr("Top talkers"));
      topTalkersGroup.setLayout(new FillLayout());
      topTalkersTable = new TrafficQueryTable(topTalkersGroup, SWT.NONE, this, "TopTalkers", i18n.tr("Reading top talkers"),
            () -> queryPointTable("TopTalkers", TrafficObserver.CAP_POINT_TOP_TALKERS));
      topTalkersTable.setNoDataMessage(i18n.tr("Not supported by traffic analyzer"));
      topTalkersTable.setSortColumn("BYTES", SWT.DOWN);
   }

   /**
    * Query point-level table if the owning observer's backend has the required capability.
    *
    * @param name table metric name
    * @param requiredCapability required backend capability
    * @return table or null if not available
    * @throws Exception on query failure
    */
   private Table queryPointTable(String name, long requiredCapability) throws Exception
   {
      ObservationPoint point = (ObservationPoint)getObject();
      if (point == null)
         return null;
      AbstractObject observer = session.findObjectById(point.getObserverId());
      if ((observer instanceof TrafficObserver) && !((TrafficObserver)observer).hasCapability(requiredCapability))
         return null;
      return session.queryTable(point.getObjectId(), DataOrigin.TRAFFIC_OBSERVER, name);
   }

   /**
    * Format throughput value in bps for display.
    *
    * @param value raw metric value
    * @return formatted text
    */
   private static String formatThroughput(String value)
   {
      try
      {
         double bps = Double.parseDouble(value);
         if (bps >= 1000000000.0)
            return String.format("%.1f Gbps", bps / 1000000000.0);
         if (bps >= 1000000.0)
            return String.format("%.1f Mbps", bps / 1000000.0);
         if (bps >= 1000.0)
            return String.format("%.1f Kbps", bps / 1000.0);
         return String.format("%.0f bps", bps);
      }
      catch(NumberFormatException e)
      {
         return value;
      }
   }

   /**
    * Refresh live stat header in background
    */
   private void refreshStats()
   {
      final long objectId = getObjectId();
      if (objectId == 0)
         return;

      new Job(i18n.tr("Reading observation point statistics"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final String[] values = new String[STAT_METRICS.length];
            for(int i = 0; i < STAT_METRICS.length; i++)
            {
               try
               {
                  values[i] = session.queryMetric(objectId, DataOrigin.TRAFFIC_OBSERVER, STAT_METRICS[i]);
               }
               catch(Exception e)
               {
                  values[i] = null;
               }
            }
            runInUIThread(() -> {
               if (statLabels[0].isDisposed() || (getObjectId() != objectId))
                  return;
               for(int i = 0; i < STAT_METRICS.length; i++)
               {
                  if (values[i] == null)
                     statLabels[i].setText("-");
                  else if (STAT_METRICS[i].startsWith("Throughput"))
                     statLabels[i].setText(formatThroughput(values[i]));
                  else
                     statLabels[i].setText(values[i]);
               }
               statLabels[0].getParent().getParent().layout(true, true);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read observation point statistics");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      refreshStats();
      activeHostsTable.refresh(null);
      l7Table.refresh(null);
      topTalkersTable.refresh(null);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if ((object != null) && isActive())
         refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      refresh();
      super.activate();
   }
}
