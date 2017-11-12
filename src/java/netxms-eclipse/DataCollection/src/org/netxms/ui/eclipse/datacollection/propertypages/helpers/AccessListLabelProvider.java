package org.netxms.ui.eclipse.datacollection.propertypages.helpers;

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.model.WorkbenchLabelProvider;

/**
 * Label provider for visibility table. 
 * Uses default workbench label provider for user object and 
 * implements own for String like with warning. 
 */
public class AccessListLabelProvider extends LabelProvider implements IColorProvider
{
   private static final Color HINT_FOREGROUND = new Color(Display.getDefault(), 192, 192, 192);
   
   private WorkbenchLabelProvider workbenchLabelProvider;
   
   /**
    * Default constructor
    */
   public AccessListLabelProvider()
   {
      workbenchLabelProvider = new WorkbenchLabelProvider();
   }    

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
    */
   @Override
   public Image getImage(Object element)
   {
      if (element instanceof String)
         return null;
      return workbenchLabelProvider.getImage(element);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
    */
   @Override
   public String getText(Object element)
   {
      if(element instanceof String)
         return (String)element;
      return workbenchLabelProvider.getText(element);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      workbenchLabelProvider.dispose();
      super.dispose();
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      if (element instanceof String)
         return HINT_FOREGROUND;
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      return null;
   }
}
