package org.netxms.ui.eclipse.datacollection.propertypages.helpers;

import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;

/**
 * Label provider for visibility table. 
 * Uses default workbench label provider for user object and 
 * implements own for String like with warning. 
 */
public class VisibilityUserListLabelProvider extends LabelProvider
{
   private WorkbenchLabelProvider workbenchLabelProvider;
   
   /**
    * Default constructor
    */
   public VisibilityUserListLabelProvider()
   {
      workbenchLabelProvider = new WorkbenchLabelProvider();
   }    

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
    */
   @Override
   public Image getImage(Object element)
   {
      if(element instanceof String)
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
}
