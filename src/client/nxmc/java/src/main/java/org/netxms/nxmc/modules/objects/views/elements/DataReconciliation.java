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
package org.netxms.nxmc.modules.objects.views.elements;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.ReconciliationStatus;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.xnap.commons.i18n.I18n;

/**
 * "Data reconciliation" overview element. Shows progress of agent offline data collection catch-up (how many cached data
 * points are still pending delivery to the server, the delivery speed, and estimated completion time).
 */
public class DataReconciliation extends TableElement
{
   private final I18n i18n = LocalizationHelper.getI18n(DataReconciliation.class);

   private ViewRefreshController refreshController;
   private ReconciliationStatus status;

   /**
    * @param parent parent composite
    * @param anchor anchor element
    * @param objectView owning view
    */
   public DataReconciliation(Composite parent, OverviewPageElement anchor, ObjectView objectView)
   {
      super(parent, anchor, objectView);
      refreshController = new ViewRefreshController(objectView, -1, () -> refresh());
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
   @Override
   protected String getTitle()
   {
      return i18n.tr("Data Reconciliation");
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      return (object instanceof AbstractNode) && ((AbstractNode)object).isDataReconciliationActive();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.TableElement#createClientArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createClientArea(Composite parent)
   {
      Control area = super.createClientArea(parent);
      refreshController.setInterval(10);
      refresh();
      return area;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#dispose()
    */
   @Override
   public void dispose()
   {
      if (refreshController != null)
         refreshController.dispose();
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.TableElement#fillTable()
    */
   @Override
   protected void fillTable()
   {
      if (status == null)
      {
         addPair(i18n.tr("Status"), i18n.tr("Loading..."));
         return;
      }

      addPair(i18n.tr("Status"), status.isActive() ? i18n.tr("In progress") : i18n.tr("Completed"));
      addPair(i18n.tr("Data points remaining"), Long.toString(status.getPendingDataPoints()));
      if (status.getOldestDataTimestamp() != null)
         addPair(i18n.tr("Oldest pending data"), DateFormatFactory.getDateTimeFormat().format(status.getOldestDataTimestamp()));

      if (status.isActive())
      {
         double rate = status.getRate();
         addPair(i18n.tr("Speed"), (rate > 0) ? String.format("%.1f %s", rate, i18n.tr("data points/sec")) : i18n.tr("calculating..."));

         long eta = status.getEstimatedSecondsRemaining();
         addPair(i18n.tr("Estimated time remaining"), (eta >= 0) ? formatDuration(eta) : i18n.tr("calculating..."));
      }
   }

   /**
    * Format duration given in seconds as a human readable string.
    *
    * @param seconds duration in seconds
    * @return formatted duration
    */
   private static String formatDuration(long seconds)
   {
      if (seconds < 60)
         return Long.toString(seconds) + "s";

      long days = seconds / 86400;
      long hours = (seconds % 86400) / 3600;
      long minutes = (seconds % 3600) / 60;
      long secs = seconds % 60;

      StringBuilder sb = new StringBuilder();
      if (days > 0)
         sb.append(days).append("d ");
      if ((hours > 0) || (days > 0))
         sb.append(hours).append("h ");
      if (days == 0)
      {
         sb.append(minutes).append("m ");
         if (hours == 0)
            sb.append(secs).append("s");
      }
      return sb.toString().trim();
   }

   /**
    * Refresh element by reading current reconciliation status from server.
    */
   private void refresh()
   {
      final NXCSession session = Registry.getSession();
      final long nodeId = getObject().getObjectId();
      Job job = new Job(i18n.tr("Reading data reconciliation status"), getObjectView()) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final ReconciliationStatus rs = session.getReconciliationStatus(nodeId);
            runInUIThread(() -> {
               if (isDisposed() || (getObject() == null) || (getObject().getObjectId() != nodeId))
                  return;
               status = rs;
               onObjectChange();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read data reconciliation status");
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
   }
}
