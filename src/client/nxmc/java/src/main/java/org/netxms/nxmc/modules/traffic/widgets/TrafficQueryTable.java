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
package org.netxms.nxmc.modules.traffic.widgets;

import org.netxms.client.Table;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.eclipse.swt.widgets.Composite;
import org.xnap.commons.i18n.I18n;

/**
 * Table viewer for live traffic data queries. Columns are built dynamically from
 * the query result; the query itself is supplied by the owning view.
 */
public class TrafficQueryTable extends BaseTableValueViewer
{
   /**
    * Data query executed on refresh (in a background job). May return null to
    * indicate that no data is available (the widget then shows noDataMessage).
    */
   public interface TableQuery
   {
      public Table read() throws Exception;
   }

   private I18n i18n;
   private String jobName;
   private TableQuery query;
   private String noDataMessage = null;

   /**
    * Create traffic query table.
    *
    * @param parent parent composite
    * @param style widget style
    * @param view owning view
    * @param configSubId configuration sub-ID for saved table settings
    * @param jobName name for the background read job
    * @param query data query
    */
   public TrafficQueryTable(Composite parent, int style, ObjectView view, String configSubId, String jobName, TableQuery query)
   {
      super(parent, style, view, configSubId, true);
      this.jobName = jobName;
      this.query = query;
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#setupLocalization()
    */
   @Override
   protected void setupLocalization()
   {
      i18n = LocalizationHelper.getI18n(TrafficQueryTable.class);
   }

   /**
    * Set message to display when the query returns no data.
    *
    * @param message message to display
    */
   public void setNoDataMessage(String message)
   {
      noDataMessage = message;
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#readData()
    */
   @Override
   protected Table readData() throws Exception
   {
      return query.read();
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#getNoDataMessage()
    */
   @Override
   protected String getNoDataMessage()
   {
      return noDataMessage;
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#getReadJobName()
    */
   @Override
   protected String getReadJobName()
   {
      return jobName;
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.widgets.BaseTableValueViewer#getReadJobErrorMessage()
    */
   @Override
   protected String getReadJobErrorMessage()
   {
      return String.format(i18n.tr("Cannot read traffic data (%s)"), jobName);
   }

   /**
    * Get current table data.
    *
    * @return current table data or null
    */
   public Table getCurrentData()
   {
      return currentData;
   }
}
