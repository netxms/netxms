/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.logviewer.views;

import java.util.ArrayList;
import java.util.List;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.LogFilter;
import org.netxms.client.log.OrderingColumn;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Ad-hoc Event Log viewer with pre-configured filter.
 * Opens with filters for event source and/or event code based on a selected event.
 */
public class AdHocEventLogView extends LogViewer
{
   private final I18n i18n = LocalizationHelper.getI18n(AdHocEventLogView.class);

   private Long sourceObjectId;
   private Integer eventCode;
   private String viewTitleSuffix;

   /**
    * Internal constructor for cloning
    */
   protected AdHocEventLogView()
   {
      super();
   }

   /**
    * Create ad-hoc event log view with specified filters.
    *
    * @param sourceObjectId source object ID to filter by (null for no source filter)
    * @param eventCode event code to filter by (null for no code filter)
    * @param viewTitleSuffix suffix for view title (e.g., "Node1", "SYS_NODE_DOWN")
    */
   public AdHocEventLogView(Long sourceObjectId, Integer eventCode, String viewTitleSuffix)
   {
      super(LocalizationHelper.getI18n(AdHocEventLogView.class).tr("Events") + " - " + viewTitleSuffix,
            "EventLog",
            ".adhoc." + (sourceObjectId != null ? sourceObjectId : "0") + "." + (eventCode != null ? eventCode : "0") + "." + System.currentTimeMillis());
      this.sourceObjectId = sourceObjectId;
      this.eventCode = eventCode;
      this.viewTitleSuffix = viewTitleSuffix;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#cloneView()
    */
   @Override
   public View cloneView()
   {
      AdHocEventLogView view = (AdHocEventLogView)super.cloneView();
      view.sourceObjectId = sourceObjectId;
      view.eventCode = eventCode;
      view.viewTitleSuffix = viewTitleSuffix;
      return view;
   }

   /**
    * @see org.netxms.nxmc.modules.logviewer.views.LogViewer#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      LogFilter filter = createEventFilter();
      queryWithFilter(filter);
      super.postContentCreate();
   }

   /**
    * Create log filter based on configured criteria.
    *
    * @return log filter
    */
   private LogFilter createEventFilter()
   {
      LogFilter filter = new LogFilter();

      if (sourceObjectId != null)
      {
         ColumnFilter sourceFilter = new ColumnFilter(ColumnFilterType.EQUALS, sourceObjectId);
         filter.setColumnFilter("event_source", sourceFilter);
      }

      if (eventCode != null)
      {
         ColumnFilter codeFilter = new ColumnFilter(ColumnFilterType.EQUALS, eventCode);
         filter.setColumnFilter("event_code", codeFilter);
      }

      // Set default ordering by timestamp descending
      List<OrderingColumn> orderingColumns = new ArrayList<>(1);
      orderingColumns.add(new OrderingColumn("event_timestamp", i18n.tr("Time"), true));
      filter.setOrderingColumns(orderingColumns);

      return filter;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#isCloseable()
    */
   @Override
   public boolean isCloseable()
   {
      return true;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getFullName()
    */
   @Override
   public String getFullName()
   {
      return getName();
   }
}
