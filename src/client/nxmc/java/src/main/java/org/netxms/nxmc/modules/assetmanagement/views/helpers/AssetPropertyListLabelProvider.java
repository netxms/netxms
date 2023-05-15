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
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.nxmc.modules.assetmanagement.views.AssetView;

/**
 * Asset properties label provider
 */
public class AssetPropertyListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private AssetPropertyReader propertyReader;

   /**
    * Create label provider.
    *
    * @param propertyReader asset property reader
    */
   public AssetPropertyListLabelProvider(AssetPropertyReader propertyReader)
   {
      super();
      this.propertyReader = propertyReader;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @SuppressWarnings("unchecked")
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex == AssetView.VALUE)
         return propertyReader.valueToImage(((Entry<String, String>)element).getKey(), ((Entry<String, String>)element).getValue());
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @SuppressWarnings("unchecked")
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      String name = ((Entry<String, String>)element).getKey();
      switch(columnIndex)
      {
         case AssetView.NAME:
            return propertyReader.getDisplayName(name);
         case AssetView.VALUE:
            return propertyReader.valueToText(name, ((Entry<String, String>)element).getValue());
         case AssetView.IS_MANDATORY:
            return propertyReader.isMandatory(name);
         case AssetView.IS_UNIQUE:
            return propertyReader.isUnique(name);
         case AssetView.SYSTEM_TYPE:
            return propertyReader.getSystemType(name);
      }
      return null;
   }
}
