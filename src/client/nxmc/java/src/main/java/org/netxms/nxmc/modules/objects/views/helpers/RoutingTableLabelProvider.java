/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.constants.RoutingProtocol;
import org.netxms.client.topology.Route;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.RoutingTableView;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for routing table view
 */
public class RoutingTableLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(RoutingTableLabelProvider.class);

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
      Route r = (Route)element;
      switch(columnIndex)
      {
         case RoutingTableView.COLUMN_DESTINATION:
            return r.getDestination().toString();
         case RoutingTableView.COLUMN_INTERFACE:
            return r.getIfName();
         case RoutingTableView.COLUMN_METRIC:
            return Integer.toString(r.getMetric());
         case RoutingTableView.COLUMN_NEXT_HOP:
            return r.getNextHop().getHostAddress();
         case RoutingTableView.COLUMN_PROTOCOL:
            return getProtocolName(r.getProtocol());
         case RoutingTableView.COLUMN_TYPE:
            return Integer.toString(r.getType());
      }
      return null;
   }

   /**
    * Get name of routing protocol.
    *
    * @param protocol routing protocol code
    * @return routing protocol name
    */
   private String getProtocolName(RoutingProtocol protocol)
   {
      switch(protocol)
      {
         case BBN_SPF_IGP:
            return "BBN SPF IGP";
         case ES_IS:
            return "ES-IS";
         case IS_IS:
            return "IS-IS";
         case LOCAL:
            return i18n.tr("Local");
         case NETMGMT:
            return i18n.tr("Network Management");
         case OTHER:
            return i18n.tr("Other");
         case UNKNOWN:
            return i18n.tr("Unknown");
         default:
            return protocol.toString();
      }
   }
}
