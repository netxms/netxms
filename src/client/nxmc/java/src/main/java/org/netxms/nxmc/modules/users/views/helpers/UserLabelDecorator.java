/**
 * 
 */
package org.netxms.nxmc.modules.users.views.helpers;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.DecorationOverlayIcon;
import org.eclipse.jface.viewers.IDecoration;
import org.eclipse.jface.viewers.ILabelDecorator;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.resources.StatusDisplayInfo;

/**
 * Decorator for user/group labels
 */
public class UserLabelDecorator implements ILabelDecorator
{
   private Map<Image, Image[]> imageCache = new HashMap<Image, Image[]>();

   /**
    * @see org.eclipse.jface.viewers.ILabelDecorator#decorateImage(org.eclipse.swt.graphics.Image, java.lang.Object)
    */
   @Override
   public Image decorateImage(Image image, Object element)
   {
      if ((image == null) || !(element instanceof AbstractUserObject))
         return null;

      int state = 0;
      if (((AbstractUserObject)element).isDisabled())
         state |= 1;
      if ((((AbstractUserObject)element).getFlags() & AbstractUserObject.SYNC_EXCEPTION) != 0)
         state |= 2;
      if (state == 0)
         return null; // Decoration is not needed

      int index = state - 1;
      Image[] decoratedImages = imageCache.get(image);
      if (decoratedImages != null)
      {
         if (decoratedImages[index] != null)
            return decoratedImages[index];
      }
      else
      {
         decoratedImages = new Image[3];
         imageCache.put(image, decoratedImages);
      }

      ImageDescriptor[] overlays = new ImageDescriptor[6];
      if ((state & 1) != 0)
         overlays[IDecoration.BOTTOM_RIGHT] = StatusDisplayInfo.getStatusOverlayImageDescriptor(ObjectStatus.DISABLED);
      if ((state & 2) != 0)
         overlays[IDecoration.TOP_LEFT] = StatusDisplayInfo.getStatusOverlayImageDescriptor(ObjectStatus.MINOR);
      DecorationOverlayIcon icon = new DecorationOverlayIcon(image, overlays);
      decoratedImages[index] = icon.createImage();
      return decoratedImages[index];
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
}
