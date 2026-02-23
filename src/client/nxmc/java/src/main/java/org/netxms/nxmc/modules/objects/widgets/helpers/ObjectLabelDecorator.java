/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.DecorationOverlayIcon;
import org.eclipse.jface.viewers.IDecoration;
import org.eclipse.jface.viewers.ILabelDecorator;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.resources.StatusDisplayInfo;

/**
 * Label decorator for objects
 */
public class ObjectLabelDecorator implements ILabelDecorator
{
   private Map<Image, Image[]> imageCache = new HashMap<Image, Image[]>();
   private boolean showChildCount = false;

   /**
    * @see org.eclipse.jface.viewers.ILabelDecorator#decorateImage(org.eclipse.swt.graphics.Image, java.lang.Object)
    */
   @Override
   public Image decorateImage(Image image, Object element)
   {
      if ((image == null) || !(element instanceof AbstractObject))
         return null;

      AbstractObject object = (AbstractObject)element;
      if (object.getStatus() == ObjectStatus.NORMAL)
         return null;

      Image[] decoratedImages = imageCache.get(image);
      int index = object.getStatus().getValue();
      if (decoratedImages != null)
      {
         if (decoratedImages[index] != null)
            return decoratedImages[index];
      }
      else
      {
         decoratedImages = new Image[9];
         imageCache.put(image, decoratedImages);
      }

      DecorationOverlayIcon icon = new DecorationOverlayIcon(image, StatusDisplayInfo.getStatusOverlayImageDescriptor(object.getStatus()), IDecoration.BOTTOM_RIGHT);
      decoratedImages[index] = icon.createImage();

      return decoratedImages[index];
   }

   /**
    * @see org.eclipse.jface.viewers.ILabelDecorator#decorateText(java.lang.String, java.lang.Object)
    */
   @Override
   public String decorateText(String text, Object element)
   {
      AbstractObject object = (AbstractObject)element;
      StringBuilder decoration = new StringBuilder();
      if (showChildCount && isContainerObject(object))
         decoration.append(" (").append(object.getChildCount()).append(')');
      if (object.isInMaintenanceMode())
         decoration.append(" [Maintenance]");
      else if ((object instanceof AbstractNode) && ((AbstractNode)object).isDecommissioned())
         decoration.append(" [Decommissioned]");
      return (decoration.length() > 0) ? text + decoration.toString() : null;
   }

   /**
    * Check if given object is a container-like object that can have children displayed as count.
    *
    * @param object object to check
    * @return true if object is a container-like object
    */
   private static boolean isContainerObject(AbstractObject object)
   {
      switch(object.getObjectClass())
      {
         case AbstractObject.OBJECT_CONTAINER:
         case AbstractObject.OBJECT_CIRCUIT:
         case AbstractObject.OBJECT_COLLECTOR:
         case AbstractObject.OBJECT_TEMPLATEGROUP:
         case AbstractObject.OBJECT_DASHBOARDGROUP:
         case AbstractObject.OBJECT_NETWORKMAPGROUP:
         case AbstractObject.OBJECT_ASSETGROUP:
         case AbstractObject.OBJECT_BUSINESSSERVICE:
         case AbstractObject.OBJECT_SERVICEROOT:
         case AbstractObject.OBJECT_TEMPLATEROOT:
         case AbstractObject.OBJECT_DASHBOARDROOT:
         case AbstractObject.OBJECT_NETWORKMAPROOT:
         case AbstractObject.OBJECT_ASSETROOT:
         case AbstractObject.OBJECT_BUSINESSSERVICEROOT:
            return true;
         default:
            return false;
      }
   }

   /**
    * @return true if child object count is shown
    */
   public boolean isShowChildCount()
   {
      return showChildCount;
   }

   /**
    * Set show child object count flag.
    *
    * @param showChildCount true to show child object count
    */
   public void setShowChildCount(boolean showChildCount)
   {
      this.showChildCount = showChildCount;
   }

   /**
    * @see org.eclipse.jface.viewers.IBaseLabelProvider#isLabelProperty(java.lang.Object, java.lang.String)
    */
   @Override
   public boolean isLabelProperty(Object element, String property)
   {
      return false;
   }

   /**
    * @see org.eclipse.jface.viewers.IBaseLabelProvider#addListener(org.eclipse.jface.viewers.ILabelProviderListener)
    */
   @Override
   public void addListener(ILabelProviderListener listener)
   {
   }

   /**
    * @see org.eclipse.jface.viewers.IBaseLabelProvider#removeListener(org.eclipse.jface.viewers.ILabelProviderListener)
    */
   @Override
   public void removeListener(ILabelProviderListener listener)
   {
   }

   /**
    * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      for(Image[] images : imageCache.values())
         for(Image image : images)
            if (image != null)
               image.dispose();
   }
}
