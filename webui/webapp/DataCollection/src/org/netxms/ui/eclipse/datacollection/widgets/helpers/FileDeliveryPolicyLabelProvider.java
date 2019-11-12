/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.ui.eclipse.datacollection.Activator;

/**
 * Label provider for file delivery policy tree
 */
public class FileDeliveryPolicyLabelProvider extends LabelProvider
{
   private Image imageFolder;
   private Image imageFile;
   
   /**
    * Constructor 
    */
   public FileDeliveryPolicyLabelProvider()
   {
      imageFolder = Activator.getImageDescriptor("icons/folder.gif").createImage();
      imageFile = Activator.getImageDescriptor("icons/file.png").createImage();
   }

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
    */
   @Override
   public Image getImage(Object element)
   {
      return ((PathElement)element).isFile() ? imageFile : imageFolder;
   }

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
    */
   @Override
   public String getText(Object element)
   {
      return ((PathElement)element).getName();
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
}
