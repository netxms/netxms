/**
 * NetXMS - open source network management system
 * Copyright (C) 2022 Raden Solutions
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
import org.netxms.client.topology.OSPFNeighbor;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.OSPFView;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for OSPF areas
 */
public class OSPFNeighborLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private I18n i18n = LocalizationHelper.getI18n(OSPFNeighborLabelProvider.class);
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
      OSPFNeighbor n = (OSPFNeighbor)element;
      switch(columnIndex)
      {
         case OSPFView.COLUMN_NEIGHBOR_AREA_ID:
            return n.isVirtual() ? n.getAreaId().getHostAddress() : "";
         case OSPFView.COLUMN_NEIGHBOR_IF_INDEX:
            return n.isVirtual() ? "" : Integer.toString(n.getInterfaceIndex());
         case OSPFView.COLUMN_NEIGHBOR_INTERFACE:
            return n.isVirtual() ? "" : session.getObjectName(n.getInterfaceId());
         case OSPFView.COLUMN_NEIGHBOR_IP_ADDRESS:
            return n.getIpAddress().getHostAddress();
         case OSPFView.COLUMN_NEIGHBOR_NODE:
            return (n.getNodeId() != 0) ? session.getObjectName(n.getNodeId()) : "";
         case OSPFView.COLUMN_NEIGHBOR_ROUTER_ID:
            return n.getRouterId().getHostAddress();
         case OSPFView.COLUMN_NEIGHBOR_STATE:
            return n.getState().getText();
         case OSPFView.COLUMN_NEIGHBOR_VIRTUAL:
            return n.isVirtual() ? i18n.tr("yes") : i18n.tr("no");
      }
      return null;
   }
}
