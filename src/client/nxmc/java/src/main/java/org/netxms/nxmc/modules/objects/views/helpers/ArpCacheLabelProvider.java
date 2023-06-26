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
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.ArpCacheEntry;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.objects.views.ArpCacheView;
import org.netxms.nxmc.tools.ViewerElementUpdater;

/**
 * Label provider for ARP cache view
 */
public class ArpCacheLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private NXCSession session = Registry.getSession();
   private TableViewer viewer;

   /**
    * Constructor
    */
   public ArpCacheLabelProvider(TableViewer viewer)
   {
      this.viewer = viewer;
   }

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
      ArpCacheEntry e = (ArpCacheEntry)element;
      switch(columnIndex)
      {
         case ArpCacheView.COLUMN_INTERFACE:
            return e.getInterfaceName();
         case ArpCacheView.COLUMN_IP_ADDRESS:
            return e.getIpAddress().getHostAddress();
         case ArpCacheView.COLUMN_MAC_ADDRESS:
            return e.getMacAddress().toString();
         case ArpCacheView.COLUMN_NODE:
            if (e.getNodeId() == 0)
               return "";
            return session.getObjectNameWithAlias(e.getNodeId());
         case ArpCacheView.COLUMN_VENDOR:
            return session.getVendorByMac(e.getMacAddress(), new ViewerElementUpdater(viewer, element));
      }
      return null;
   }
}
