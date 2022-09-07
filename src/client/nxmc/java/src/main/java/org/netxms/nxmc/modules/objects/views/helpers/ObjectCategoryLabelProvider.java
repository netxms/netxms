/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Victor Kirhenshtein
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

import java.util.UUID;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.base.NXCommon;
import org.netxms.client.objects.ObjectCategory;
import org.netxms.nxmc.modules.objects.views.ObjectCategoryManager;

/**
 * Label provider for object category list
 */
public class ObjectCategoryLabelProvider extends LabelProvider implements ITableLabelProvider
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
      ObjectCategory category = (ObjectCategory)element;
      switch(columnIndex)
      {
         case ObjectCategoryManager.COL_ICON:
            return imageText(category.getIcon());
         case ObjectCategoryManager.COL_ID:
            return Integer.toString(category.getId());
         case ObjectCategoryManager.COL_MAP_IMAGE:
            return imageText(category.getMapImage());
         case ObjectCategoryManager.COL_NAME:
            return category.getName();
      }
      return null;
   }

   /**
    * Get text for image UUID.
    *
    * @param image image UUID
    * @return display text
    */
   private static String imageText(UUID image)
   {
      return (image == null) || image.equals(NXCommon.EMPTY_GUID) ? "" : image.toString();
   }
}
