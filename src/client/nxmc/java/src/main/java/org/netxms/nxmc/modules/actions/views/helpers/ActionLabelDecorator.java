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
package org.netxms.nxmc.modules.actions.views.helpers;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.DecorationOverlayIcon;
import org.eclipse.jface.viewers.IDecoration;
import org.eclipse.jface.viewers.ILabelDecorator;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.ServerAction;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.nxmc.resources.StatusDisplayInfo;

/**
 * Label decorator for server actions
 */
public class ActionLabelDecorator implements ILabelDecorator
{
   private Map<Image, Image> imageCache = new HashMap<>();

   /**
    * @see org.eclipse.jface.viewers.ILabelDecorator#decorateImage(org.eclipse.swt.graphics.Image, java.lang.Object)
    */
   @Override
   public Image decorateImage(Image image, Object element)
   {
      if ((image == null) || !(element instanceof ServerAction))
         return null;

      ServerAction action = (ServerAction)element;
      if (!action.isDisabled())
         return null;

      Image decoratedImage = imageCache.get(image);
      if (decoratedImage == null)
      {
         DecorationOverlayIcon icon = new DecorationOverlayIcon(image, StatusDisplayInfo.getStatusOverlayImageDescriptor(ObjectStatus.DISABLED), IDecoration.TOP_RIGHT);
         decoratedImage = icon.createImage();
         imageCache.put(image, decoratedImage);
      }
      return decoratedImage;
   }

   /**
    * @see org.eclipse.jface.viewers.ILabelDecorator#decorateText(java.lang.String, java.lang.Object)
    */
   @Override
   public String decorateText(String text, Object element)
   {
      return null;
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
      for(Image i : imageCache.values())
         i.dispose();
   }
}
