package org.netxms.ui.eclipse.console;

import org.eclipse.jface.viewers.ColumnViewer;

public class UserRefreshRunnable implements Runnable
{
   private ColumnViewer viewer;
   private Object element;
   
   /**
    * Constructor
    */
   public UserRefreshRunnable(ColumnViewer viewer, Object element)
   {
      this.viewer = viewer;
      this.element = element;
   }

   @Override
   public void run()
   {
      viewer.getControl().getDisplay().asyncExec(new Runnable() {
         @Override
         public void run()
         {
            viewer.refresh(element, true);
         }
      });
   }

}
