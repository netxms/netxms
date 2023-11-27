/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.widgets.helpers;

import java.util.function.Consumer;
import org.eclipse.jface.viewers.DecoratingLabelProvider;
import org.eclipse.swt.graphics.Color;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.resources.ThemeEngine;

/**
 * Label provider for objects that decorates object elements according to object status
 */
public class DecoratingObjectLabelProvider extends DecoratingLabelProvider
{
   private Color maintenanceColor = ThemeEngine.getForegroundColor("ObjectTree.Maintenance");

   /**
    * Create new decorating label provider
    */
   public DecoratingObjectLabelProvider()
   {
      this(null, false);
   }

   /**
    * Create decorating label provider with image update callback.
    * 
    * @param imageUpdateCallback callback to be called when missing image is loaded from server
    */
   public DecoratingObjectLabelProvider(Consumer<AbstractObject> imageUpdateCallback)
   {
      this(imageUpdateCallback, false);
   }

   /**
    * Create new decorating label provider
    * 
    * @param showFullPath true to show full path to object as part of object name
    */
   public DecoratingObjectLabelProvider(boolean showFullPath)
   {
      this(null, showFullPath);
   }

   /**
    * Create decorating label provider with image update callback and option to show full path to object.
    * 
    * @param imageUpdateCallback callback to be called when missing image is loaded from server
    * @param showFullPath true to show full path to object as part of object name
    */
   public DecoratingObjectLabelProvider(Consumer<AbstractObject> imageUpdateCallback, boolean showFullPath)
   {
      super(new BaseObjectLabelProvider(imageUpdateCallback, showFullPath), new ObjectLabelDecorator());
   }

   /**
    * @see org.eclipse.jface.viewers.DecoratingLabelProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      if (((AbstractObject)element).isInMaintenanceMode())
         return maintenanceColor;
      return null;
   }
}
