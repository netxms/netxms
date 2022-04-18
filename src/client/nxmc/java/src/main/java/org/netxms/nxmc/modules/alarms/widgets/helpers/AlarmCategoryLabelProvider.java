/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2022 RadenSolutions
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
package org.netxms.nxmc.modules.alarms.widgets.helpers;

import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.events.AlarmCategory;
import org.netxms.nxmc.modules.alarms.widgets.AlarmCategoryList;

/**
 * Label provider for alarm categoryies
 * 
 */
public class AlarmCategoryLabelProvider implements ITableLabelProvider
{
   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
      //TODO: return (columnIndex == 0) ? getImage(element) : null;
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      switch(columnIndex)
      {
         case AlarmCategoryList.COLUMN_ID:
            return Long.toString(((AlarmCategory)element).getId());
         case AlarmCategoryList.COLUMN_NAME:
            return ((AlarmCategory)element).getName();
         case AlarmCategoryList.COLUMN_DESCRIPTION:
            return ((AlarmCategory)element).getDescription();
      }
      return null;
   }

   @Override
   public void addListener(ILabelProviderListener listener)
   {
      // TODO Auto-generated method stub
      
   }

   @Override
   public void dispose()
   {
      // TODO Auto-generated method stub
      
   }

   @Override
   public boolean isLabelProperty(Object element, String property)
   {
      // TODO Auto-generated method stub
      return false;
   }

   @Override
   public void removeListener(ILabelProviderListener listener)
   {
      // TODO Auto-generated method stub
      
   }

}
