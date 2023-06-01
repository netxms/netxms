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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.assetmanagement.views.AssetManagementSchemaManager;

/**
 * Asset attribute comparator
 */
public class AssetAttributeComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      AssetAttribute attr1 = (AssetAttribute)e1;
      AssetAttribute attr2 = (AssetAttribute)e2;
      int result = 0; 

      switch((Integer) ((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
      {
         case AssetManagementSchemaManager.COLUMN_DATA_TYPE:
            result = AssetAttributeListLabelProvider.DATA_TYPES[attr1.getDataType().getValue()].compareTo(AssetAttributeListLabelProvider.DATA_TYPES[attr2.getDataType().getValue()]);
            break;
         case AssetManagementSchemaManager.COLUMN_DISPLAY_NAME:
            result = attr1.getDisplayName().compareToIgnoreCase(attr2.getDisplayName());
            break;
         case AssetManagementSchemaManager.COLUMN_HAS_SCRIPT:
            result = Boolean.compare(!attr1.getAutofillScript().isEmpty(), !attr2.getAutofillScript().isEmpty());
            break;
         case AssetManagementSchemaManager.COLUMN_IS_HIDDEN:
            result = Boolean.compare(attr1.isHidden(), attr2.isHidden());
            break;
         case AssetManagementSchemaManager.COLUMN_IS_MANDATORY:
            result = Boolean.compare(attr1.isMandatory(), attr2.isMandatory());
            break;
         case AssetManagementSchemaManager.COLUMN_IS_UNIQUE:
            result = Boolean.compare(attr1.isUnique(), attr2.isUnique());
            break;
         case AssetManagementSchemaManager.COLUMN_NAME:
            result = attr1.getName().compareToIgnoreCase(attr2.getName());
            break;
         case AssetManagementSchemaManager.COLUMN_RANGE_MAX:
            result = Integer.compare(attr1.getRangeMax(), attr2.getRangeMax());
            break;
         case AssetManagementSchemaManager.COLUMN_RANGE_MIN:
            result = Integer.compare(attr1.getRangeMin(), attr2.getRangeMin());
            break;
         case AssetManagementSchemaManager.COLUMN_SYSTEM_TYPE:
            result = AssetAttributeListLabelProvider.SYSTEM_TYPE[attr1.getSystemType().getValue()].compareTo(
                  AssetAttributeListLabelProvider.SYSTEM_TYPE[attr2.getSystemType().getValue()]);
            break;
      }      

      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : - result;
   }
}
