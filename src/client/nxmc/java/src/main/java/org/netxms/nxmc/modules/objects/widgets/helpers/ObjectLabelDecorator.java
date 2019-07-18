package org.netxms.nxmc.modules.objects.widgets.helpers;

import org.eclipse.jface.viewers.ILabelDecorator;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractObject;

public class ObjectLabelDecorator implements ILabelDecorator
{
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

      return null;
   }

   @Override
   public String decorateText(String text, Object element)
   {
      // TODO Auto-generated method stub
      return null;
   }

   @Override
   public boolean isLabelProperty(Object element, String property)
   {
      return false;
   }

   @Override
   public void addListener(ILabelProviderListener listener)
   {
   }

   @Override
   public void removeListener(ILabelProviderListener listener)
   {
   }

   @Override
   public void dispose()
   {
   }
}
