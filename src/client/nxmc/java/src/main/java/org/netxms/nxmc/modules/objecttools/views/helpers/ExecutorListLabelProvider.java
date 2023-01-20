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
package org.netxms.nxmc.modules.objecttools.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.constants.Severity;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.netxms.nxmc.modules.objecttools.widgets.AbstractObjectToolExecutor;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;

/**
 * Label provider for object tool executors list
 */
public class ExecutorListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private BaseObjectLabelProvider objectLabelProvider;

   public ExecutorListLabelProvider()
   {
      objectLabelProvider = new BaseObjectLabelProvider();      
   }
   
   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      AbstractObjectToolExecutor e = (AbstractObjectToolExecutor)element;
      switch(columnIndex)
      {
         case 0:
            return objectLabelProvider.getImage(e.getObject());
         case 1:
            if (e.isRunning())
               return SharedIcons.IMG_EXECUTE;
            if (e.isFailed())
               return StatusDisplayInfo.getStatusImage(Severity.CRITICAL);
            return StatusDisplayInfo.getStatusImage(Severity.NORMAL);
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      AbstractObjectToolExecutor e = (AbstractObjectToolExecutor)element;
      switch(columnIndex)
      {
         case 0:
            return e.getObject().getObjectName();
         case 1:
            return e.isRunning() ? "Running" : (e.isFailed() ? "Failed (" + e.getFailureReason() + ")" : "Completed");
      }
      return null;
   }

   @Override
   public void dispose()
   {
      objectLabelProvider.dispose();
      super.dispose();
   }
}
