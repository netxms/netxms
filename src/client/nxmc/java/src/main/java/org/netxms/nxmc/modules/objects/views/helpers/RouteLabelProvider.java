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
import org.netxms.client.NXCSession;
import org.netxms.client.topology.HopInfo;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.RouteView;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for route view
 */
public class RouteLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(RouteLabelProvider.class);

   private NXCSession session = Registry.getSession();

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
      HopInfo hop = (HopInfo)element;
      switch(columnIndex)
      {
         case RouteView.COLUMN_HOP:
            return Integer.toString(hop.getIndex());
         case RouteView.COLUMN_NAME:
            return hop.getName();
         case RouteView.COLUMN_NEXT_HOP:
            return (hop.getNextHop() != null) ? hop.getNextHop().getHostAddress() : "";
         case RouteView.COLUMN_NODE:
            return (hop.getNodeId() > 0) ? session.getObjectName(hop.getNodeId()) : i18n.tr("??");
         case RouteView.COLUMN_TYPE:
            switch(hop.getType())
            {
               case HopInfo.DUMMY:
                  return "";
               case HopInfo.PROXY:
                  return i18n.tr("Proxy");
               case HopInfo.ROUTE:
                  return i18n.tr("Route");
               case HopInfo.VPN:
                  return i18n.tr("VPN");
            }
            break;
      }
      return null;
   }
}
