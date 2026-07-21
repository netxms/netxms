/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.datacollection.NetconfQueryDefinition;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.views.NetconfQueryManager;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for NETCONF query definitions
 */
public class NetconfQueryLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(NetconfQueryLabelProvider.class);

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      NetconfQueryDefinition d = (NetconfQueryDefinition)element;
      switch(columnIndex)
      {
         case NetconfQueryManager.COLUMN_ID:
            return Integer.toString(d.getId());
         case NetconfQueryManager.COLUMN_NAME:
            return d.getName();
         case NetconfQueryManager.COLUMN_DATASTORE:
            return getDatastoreName(d.getDatastore());
         case NetconfQueryManager.COLUMN_FILTER_TYPE:
            return getFilterTypeName(d.getFilterType());
         case NetconfQueryManager.COLUMN_RETENTION_TIME:
            return Integer.toString(d.getCacheRetentionTime());
         case NetconfQueryManager.COLUMN_TIMEOUT:
            return Integer.toString(d.getRequestTimeout());
         case NetconfQueryManager.COLUMN_DESCRIPTION:
            return d.getDescription();
      }
      return null;
   }

   /**
    * Get display name for given datastore code.
    *
    * @param datastore datastore code
    * @return display name for given datastore code
    */
   public String getDatastoreName(int datastore)
   {
      switch(datastore)
      {
         case NetconfQueryDefinition.DATASTORE_OPERATIONAL:
            return i18n.tr("Operational (get)");
         case NetconfQueryDefinition.DATASTORE_RUNNING:
            return i18n.tr("Running");
         case NetconfQueryDefinition.DATASTORE_CANDIDATE:
            return i18n.tr("Candidate");
         case NetconfQueryDefinition.DATASTORE_STARTUP:
            return i18n.tr("Startup");
      }
      return Integer.toString(datastore);
   }

   /**
    * Get display name for given filter type code.
    *
    * @param filterType filter type code
    * @return display name for given filter type code
    */
   public String getFilterTypeName(int filterType)
   {
      switch(filterType)
      {
         case NetconfQueryDefinition.FILTER_NONE:
            return i18n.tr("None");
         case NetconfQueryDefinition.FILTER_SUBTREE:
            return i18n.tr("Subtree");
         case NetconfQueryDefinition.FILTER_XPATH:
            return i18n.tr("XPath");
      }
      return Integer.toString(filterType);
   }
}
