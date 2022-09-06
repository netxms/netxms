/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Filter for scheduled tasks
 */
public class ScheduledTaskFilter extends ViewerFilter implements AbstractViewerFilter
{
   private NXCSession session = Registry.getSession();
   private String filterString = null;
   private boolean showSystemTasks = false;
   private boolean showDisabledTasks = true;
   private boolean showCompletedTasks = true;

   /**
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
      if ((filterString != null) && !filterString.isEmpty())
      {
         return task.getComments().toLowerCase().contains(filterString) || 
               task.getParameters().toLowerCase().contains(filterString) || 
               task.getTaskHandlerId().toLowerCase().contains(filterString) ||
               ((task.getObjectId() != 0) && session.getObjectName(task.getObjectId()).toLowerCase().contains(filterString));
      }
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

   /**
    * @see org.netxms.nxmc.base.views.AbstractViewerFilter#setFilterString(java.lang.String)
    */
   @Override
   public void setFilterString(String filterString)
   {
      this.filterString = filterString;
   }
}
