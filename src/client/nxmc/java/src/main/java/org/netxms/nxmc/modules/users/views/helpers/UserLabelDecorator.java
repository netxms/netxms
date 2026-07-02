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
import org.netxms.client.users.User;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;

/**
 * Decorator for user/group labels
 */
public class UserLabelDecorator implements ILabelDecorator
{
   private Map<ImageDescriptor, Image[]> imageCache = new HashMap<ImageDescriptor, Image[]>();

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

      // Do not use provided image for composition - in web client it can be bound to another UI
      // session, and reading image data from it will fail once the owning session is closed.
      // Select matching image descriptor and create base image on the current display instead.
      ImageDescriptor baseDescriptor = (element instanceof User) ? (((User)element).isServiceAccount() ? SharedIcons.SERVICE : SharedIcons.USER) : SharedIcons.GROUP;

      int index = state - 1;
      Image[] decoratedImages = imageCache.get(baseDescriptor);
      if (decoratedImages != null)
      {
         if (decoratedImages[index] != null)
            return decoratedImages[index];
      }
      else
      {
         decoratedImages = new Image[3];
         imageCache.put(baseDescriptor, decoratedImages);
      }

      ImageDescriptor[] overlays = new ImageDescriptor[6];
      if ((state & 1) != 0)
         overlays[IDecoration.BOTTOM_RIGHT] = StatusDisplayInfo.getStatusOverlayImageDescriptor(ObjectStatus.DISABLED);
      if ((state & 2) != 0)
         overlays[IDecoration.TOP_LEFT] = StatusDisplayInfo.getStatusOverlayImageDescriptor(ObjectStatus.MINOR);
      Image baseImage = baseDescriptor.createImage();
      DecorationOverlayIcon icon = new DecorationOverlayIcon(baseImage, overlays);
      decoratedImages[index] = icon.createImage();
      baseImage.dispose();
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
