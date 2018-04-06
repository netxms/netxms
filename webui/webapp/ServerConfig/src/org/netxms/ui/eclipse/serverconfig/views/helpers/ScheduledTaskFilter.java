/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.ScheduledTask;

/**
 * Filter for scheduled tasks
 */
public class ScheduledTaskFilter extends ViewerFilter
{
   private boolean showSystemTasks = false;
   private boolean showDisabledTasks = true;
   private boolean showCompletedTasks = true;

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      ScheduledTask task = (ScheduledTask)element;
      if (!showSystemTasks && task.isSystem())
         return false;
      if (!showDisabledTasks && task.isDisabled())
         return false;
      if (!showCompletedTasks && task.isCompleted() && !task.isRecurring())
         return false;
      return true;
   }

   /**
    * @return the showSystemTasks
    */
   public boolean isShowSystemTasks()
   {
      return showSystemTasks;
   }

   /**
    * @param showSystemTasks the showSystemTasks to set
    */
   public void setShowSystemTasks(boolean showSystemTasks)
   {
      this.showSystemTasks = showSystemTasks;
   }

   /**
    * @return the showDisabledTasks
    */
   public boolean isShowDisabledTasks()
   {
      return showDisabledTasks;
   }

   /**
    * @param showDisabledTasks the showDisabledTasks to set
    */
   public void setShowDisabledTasks(boolean showDisabledTasks)
   {
      this.showDisabledTasks = showDisabledTasks;
   }

   /**
    * @return the showCompletedTasks
    */
   public boolean isShowCompletedTasks()
   {
      return showCompletedTasks;
   }

   /**
    * @param showCompletedTasks the showCompletedTasks to set
    */
   public void setShowCompletedTasks(boolean showCompletedTasks)
   {
      this.showCompletedTasks = showCompletedTasks;
   }
}
