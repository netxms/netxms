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
package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.modules.datacollection.widgets.FileDeliveryPolicyEditor;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Label provider for file delivery policy tree
 */
public class FileDeliveryPolicyLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private Image imageFolder;
   private Image imageFile;
   
   /**
    * Constructor 
    */
   public FileDeliveryPolicyLabelProvider()
   {
      imageFolder = ResourceManager.getImageDescriptor("icons/folder.gif").createImage();
      imageFile = ResourceManager.getImageDescriptor("icons/file.png").createImage();
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex == 0)
         return ((PathElement)element).isFile() ? imageFile : imageFolder;
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      imageFile.dispose();
      imageFolder.dispose();
      super.dispose();
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      switch(columnIndex)
      {
         case FileDeliveryPolicyEditor.COLUMN_NAME:
            return ((PathElement)element).getName();
         case FileDeliveryPolicyEditor.COLUMN_GUID:
            return ((PathElement)element).getGuid() != null ? ((PathElement)element).getGuid().toString() : null;
         case FileDeliveryPolicyEditor.COLUMN_DATE:
            if (!((PathElement)element).isFile())
               return null;
            return ((PathElement)element).getCreationTime() != null ? DateFormatFactory.getDateTimeFormat().format(((PathElement)element).getCreationTime()) : null;
         case FileDeliveryPolicyEditor.COLUMN_USER:
             return ((PathElement)element).getOwner();
         case FileDeliveryPolicyEditor.COLUMN_GROUP:
             return ((PathElement)element).getOwnerGroup();
         case FileDeliveryPolicyEditor.COLUMN_PERMISSIONS:
             return ((PathElement)element).getPermissionsAsString();
      }
      return null;
   }
}