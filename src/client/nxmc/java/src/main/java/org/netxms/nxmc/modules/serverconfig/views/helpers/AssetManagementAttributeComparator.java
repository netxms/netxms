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
package org.netxms.nxmc.modules.serverconfig.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.asset.AssetManagementAttribute;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.serverconfig.views.AssetManagementAttributesView;


/**
 *  Asset management attribute comparator
 */
public class AssetManagementAttributeComparator extends ViewerComparator
{

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      AssetManagementAttribute attr1 = (AssetManagementAttribute)e1;
      AssetManagementAttribute attr2 = (AssetManagementAttribute)e2;
      int result = 0; 
      
      switch((Integer) ((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
      {
         case AssetManagementAttributesView.NAME:
            result = attr1.getName().compareToIgnoreCase(attr2.getName());
            break;
         case AssetManagementAttributesView.DISPLAY_NAME:
            result = attr1.getDisplayName().compareToIgnoreCase(attr2.getDisplayName());
            break;
         case AssetManagementAttributesView.DATA_TYPE:
            result = AssetManagementAttributeLabelProvider.DATA_TYPES[attr1.getDataType().getValue()].compareTo(
                  AssetManagementAttributeLabelProvider.DATA_TYPES[attr2.getDataType().getValue()]);
            break;
         case AssetManagementAttributesView.IS_MANDATORY:
            result = Boolean.compare(attr1.isMandatory(), attr2.isMandatory());
            break;
         case AssetManagementAttributesView.IS_UNIQUE:
            result = Boolean.compare(attr1.isUnique(), attr2.isUnique());
            break;
         case AssetManagementAttributesView.HAS_SCRIPT:
            result = Boolean.compare(!attr1.getAutofillScript().isEmpty(), !attr2.getAutofillScript().isEmpty());
            break;
         case AssetManagementAttributesView.RANGE_MIN:
            result = Integer.compare(attr1.getRangeMin(), attr2.getRangeMin());
            break;
         case AssetManagementAttributesView.RANGE_MAX:
            result = Integer.compare(attr1.getRangeMax(), attr2.getRangeMax());
            break;
         case AssetManagementAttributesView.SYSTEM_TYPE:
            result = AssetManagementAttributeLabelProvider.SYSTEM_TYPE[attr1.getSystemType().getValue()].compareTo(
                  AssetManagementAttributeLabelProvider.SYSTEM_TYPE[attr2.getSystemType().getValue()]);
            break;
      }      
      
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : - result;
   }
   
}
