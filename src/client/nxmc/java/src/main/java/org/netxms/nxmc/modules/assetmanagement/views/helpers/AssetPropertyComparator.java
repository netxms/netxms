/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.assetmanagement.views.helpers;

import java.util.Map.Entry;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.assetmanagement.views.AssetView;

/**
 * Asset property comparator
 */
public class AssetPropertyComparator extends ViewerComparator
{
   private AssetPropertyListLabelProvider labelProvider;

   /**
    * Asset instance comparator
    * 
    * @param labelProvider asset instance comparator
    */
   public AssetPropertyComparator(AssetPropertyListLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @SuppressWarnings("unchecked")
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      final Entry<String,String> o1 = (Entry<String, String>)e1;
      final Entry<String,String> o2 = (Entry<String, String>)e2;

      int result = 0;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
      {
         case AssetView.NAME:
            result = labelProvider.getName(o1.getKey()).compareToIgnoreCase(labelProvider.getName(o2.getKey()));
            break;
         case AssetView.VALUE:
            result = labelProvider.getPropertyValue(o1).compareToIgnoreCase(labelProvider.getPropertyValue(o2));
            break;
         case AssetView.IS_MANDATORY:
            result = labelProvider.isMandatory(o1.getKey()).compareToIgnoreCase(labelProvider.isMandatory(o2.getKey()));
            break;
         case AssetView.IS_UNIQUE:
            result = labelProvider.isUnique(o1.getKey()).compareToIgnoreCase(labelProvider.isUnique(o2.getKey()));
            break;
         case AssetView.SYSTEM_TYPE:
            result = labelProvider.getSystemType(o1.getKey()).compareToIgnoreCase(labelProvider.getSystemType(o2.getKey()));  
            break;      
      }

      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }   
}
