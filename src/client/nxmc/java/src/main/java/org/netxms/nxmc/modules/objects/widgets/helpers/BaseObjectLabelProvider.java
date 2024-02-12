/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import java.util.UUID;
import java.util.function.Consumer;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.UnknownObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.imagelibrary.ImageProvider;
import org.netxms.nxmc.modules.objects.ObjectIcons;
import org.netxms.nxmc.resources.SharedIcons;

/**
 * Base label provider for NetXMS objects
 */
public class BaseObjectLabelProvider extends LabelProvider
{
   private ObjectIcons icons = Registry.getSingleton(ObjectIcons.class);
   private boolean showFullPath;
   private Consumer<AbstractObject> imageUpdateCallback;

   /**
    * Create default label provider
    */
   public BaseObjectLabelProvider()
   {
      this(null, false);
   }

   /**
    * Create label provider with image update callback.
    * 
    * @param imageUpdateCallback callback to be called when missing image is loaded from server
    */
   public BaseObjectLabelProvider(Consumer<AbstractObject> imageUpdateCallback)
   {
      this(imageUpdateCallback, false);
   }

   /**
    * Create label provider with option to show full path to object.
    *
    * @param showFullPath true to show full path to object as part of object name
    */
   public BaseObjectLabelProvider(boolean showFullPath)
   {
      this(null, showFullPath);
   }

   /**
    * Create label provider with image update callback and option to show full path to object.
    * 
    * @param imageUpdateCallback callback to be called when missing image is loaded from server
    * @param showFullPath true to show full path to object as part of object name
    */
   public BaseObjectLabelProvider(Consumer<AbstractObject> imageUpdateCallback, boolean showFullPath)
   {
      this.showFullPath = showFullPath;
      this.imageUpdateCallback = imageUpdateCallback;
   }

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
    */
   @Override
   public Image getImage(Object element)
   {
      if (element instanceof UnknownObject)
         return SharedIcons.IMG_UNKNOWN_OBJECT;

      UUID iconId = ((AbstractObject)element).getIcon();
      if (iconId != null)
      {
         Image icon = ImageProvider.getInstance().getObjectIcon(iconId, (imageUpdateCallback != null) ? () -> imageUpdateCallback.accept((AbstractObject)element) : null);
         if (icon != null)
            return icon;
      }

      return icons.getImage(((AbstractObject)element).getObjectClass());
   }

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
    */
   @Override
   public String getText(Object element)
   {
      if (showFullPath)
         return ((AbstractObject)element).getObjectNameWithPath();         
      else
         return ((AbstractObject)element).getNameWithAlias();
   }

   /**
    * Get image update callback.
    *
    * @return image update callback
    */
   public Consumer<AbstractObject> getImageUpdateCallback()
   {
      return imageUpdateCallback;
   }

   /**
    * Set image update callback.
    *
    * @param imageUpdateCallback new image update callback
    */
   public void setImageUpdateCallback(Consumer<AbstractObject> imageUpdateCallback)
   {
      this.imageUpdateCallback = imageUpdateCallback;
   }
}
