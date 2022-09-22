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
import org.netxms.client.topology.OSPFArea;
import org.netxms.nxmc.modules.objects.views.OSPFView;

/**
 * Label provider for OSPF areas
 */
public class OSPFAreaLabelProvider extends LabelProvider implements ITableLabelProvider
{
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
      OSPFArea area = (OSPFArea)element;
      switch(columnIndex)
      {
         case OSPFView.COLUMN_AREA_ABR:
            return Integer.toString(area.getAreaBorderRouterCount());
         case OSPFView.COLUMN_AREA_ASBR:
            return Integer.toString(area.getAsBorderRouterCount());
         case OSPFView.COLUMN_AREA_ID:
            return area.getId().getHostAddress();
         case OSPFView.COLUMN_AREA_LSA:
            return Integer.toString(area.getLsaCount());
      }
      return null;
   }
}
