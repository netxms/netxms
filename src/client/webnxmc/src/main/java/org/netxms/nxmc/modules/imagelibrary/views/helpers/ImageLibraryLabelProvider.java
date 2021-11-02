/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.imagelibrary.views.helpers;

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.LibraryImage;
import org.netxms.nxmc.modules.imagelibrary.views.ImageLibrary;

/**
 * Label provider for image library
 */
public class ImageLibraryLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   private final Color categoryBackgroundColor = new Color(Display.getCurrent(), 255, 254, 194);
         
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
      if (element instanceof ImageCategory)
         return (columnIndex == 0) ? ((ImageCategory)element).getName() : null;
         
      LibraryImage image = (LibraryImage)element;
      switch(columnIndex)
      {
         case ImageLibrary.COLUMN_GUID:
            return image.getGuid().toString();
         case ImageLibrary.COLUMN_NAME:
            return image.getName();
         case ImageLibrary.COLUMN_PROTECTED:
            return image.isProtected() ? "Yes" : "No";
         case ImageLibrary.COLUMN_TYPE:
            return image.getMimeType();
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
    */
   @Override
   public String getText(Object element)
   {
      return getColumnText(element, 0);
   }

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      categoryBackgroundColor.dispose();
      super.dispose();
   }

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      if (element instanceof ImageCategory)
         return categoryBackgroundColor;
      return null;
   }
}
